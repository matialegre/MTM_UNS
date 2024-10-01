// Definición de pines
const int PUL_PIN = 16; // Pin para PUL (Pulso)
const int DIR_PIN = 17; // Pin para DIR (Dirección)

// Parámetros del motor y driver
const int microstepsPerRevolution = 6400; // 200 pasos * 32 microstepping

// Variables de control
volatile bool continuousMove = false;
volatile float continuousRPM = 0;
unsigned long lastStepTime = 0;

// Variables de aceleración
bool accelerating = false;
float initialRPM = 0;
float finalRPM = 0;
unsigned long accelerationStartTime = 0;
unsigned long accelerationDuration = 0;
String accelerationMode = ""; // "linear" o "s-curve"

void setup() {
  // Configuración de pines
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // Configuración del puerto serie
  Serial.begin(115200);
  Serial.println("Control de Motor Paso a Paso Iniciado.");
  Serial.println("Comandos disponibles:");
  Serial.println("M<grados>,<rpm> - Mover N grados a una velocidad específica.");
  Serial.println("S<rpm> - Establecer velocidad continua.");
  Serial.println("P - Detener movimiento continuo.");
  Serial.println("AL<rpmInicial>,<rpmFinal>,<tiempoSegundos> - Aceleración lineal de RPM inicial a RPM final en un tiempo específico.");
  Serial.println("AS<rpmInicial>,<rpmFinal>,<tiempoSegundos> - Aceleración curva S de RPM inicial a RPM final en un tiempo específico.");
  Serial.println("Ejemplos:");
  Serial.println("M90,30 - Mover 90 grados a 30 RPM.");
  Serial.println("S60 - Establecer velocidad continua a 60 RPM.");
  Serial.println("P - Detener el motor.");
  Serial.println("AL5,10,100 - Aceleración lineal de 5 RPM a 10 RPM en 100 segundos.");
  Serial.println("AS5,10,100 - Aceleración curva S de 5 RPM a 10 RPM en 100 segundos.");
}

void loop() {
  // Verificar si hay datos disponibles en el puerto serie
  if (Serial.available() > 0) {
    parseSerialInput();
  }

  unsigned long currentTime = micros();

  if (accelerating) {
    // Calcular el tiempo transcurrido desde que inició la aceleración
    unsigned long elapsedTime = currentTime - accelerationStartTime;
    if (elapsedTime >= accelerationDuration) {
      // Periodo de aceleración finalizado, establecer a finalRPM y detener aceleración
      accelerating = false;
      if (finalRPM != 0) {
        continuousRPM = finalRPM;
        continuousMove = true;
      } else {
        continuousMove = false;
        continuousRPM = 0;
      }
      lastStepTime = currentTime;
    } else {
      // Calcular currentRPM basado en el modo de aceleración
      float currentRPM = 0;
      float t = (float)elapsedTime / accelerationDuration; // Tiempo normalizado [0,1]

      if (accelerationMode == "linear") {
        // Aceleración lineal
        currentRPM = initialRPM + (finalRPM - initialRPM) * t;
      } else if (accelerationMode == "s-curve") {
        // Aceleración curva S usando función coseno
        float sCurveFactor = (1 - cos(PI * t)) / 2; // Curva S
        currentRPM = initialRPM + (finalRPM - initialRPM) * sCurveFactor;
      }

      // Calcular frecuencia y periodo
      float frequency = (currentRPM * microstepsPerRevolution) / 60.0;
      if (frequency <= 0) frequency = 1; // Evitar división por cero o frecuencia negativa
      float period = (1.0 / frequency) * 1000000.0; // Periodo en microsegundos
      if (period < 200) period = 200; // Periodo mínimo para evitar pasos demasiado rápidos

      if (currentTime - lastStepTime >= period / 2) {
        lastStepTime = currentTime;
        digitalWrite(PUL_PIN, !digitalRead(PUL_PIN));
      }
    }
  } else if (continuousMove) {
    // Movimiento continuo
    float frequency = (continuousRPM * microstepsPerRevolution) / 60.0;
    if (frequency <= 0) frequency = 1;
    float period = (1.0 / frequency) * 1000000.0;
    if (period < 200) period = 200;
    if (currentTime - lastStepTime >= period / 2) {
      lastStepTime = currentTime;
      digitalWrite(PUL_PIN, !digitalRead(PUL_PIN));
    }
  }
}

// Función para mover un ángulo específico a una velocidad específica
void moveDegrees(float degrees, float rpm) {
  int steps = (degrees / 360.0) * microstepsPerRevolution;
  float frequency = (rpm * microstepsPerRevolution) / 60.0;
  float period = (1.0 / frequency) * 1000000.0;

  // Asegurar que el periodo no sea demasiado pequeño
  if (period < 200) period = 200; // Valor mínimo para evitar errores

  // Generación de pulsos
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(period / 2);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(period / 2);
  }
}

// Función para establecer una velocidad continua
void setSpeed(float rpm) {
  continuousRPM = rpm;
  continuousMove = true;
  lastStepTime = micros();
}

// Función para detener el movimiento continuo
void stopMotor() {
  continuousMove = false;
  continuousRPM = 0;
  accelerating = false;
}

// Función para manejar la aceleración
void accelerate(float initRPM, float finRPM, float timeSec, String mode) {
  initialRPM = initRPM;
  finalRPM = finRPM;
  accelerationDuration = timeSec * 1000000.0; // Convertir segundos a microsegundos
  accelerationStartTime = micros();
  accelerating = true;
  continuousMove = false; // Asegurar que continuousMove esté apagado durante la aceleración
  lastStepTime = micros();
  accelerationMode = mode; // Establecer el modo de aceleración
}

// Función para analizar la entrada del usuario
void parseSerialInput() {
  String input = Serial.readStringUntil('\n');
  input.trim(); // Eliminar espacios en blanco

  if (input.length() == 0) return;

  String command = input.substring(0, 2);
  input = input.substring(2); // Eliminar el comando de la cadena

  if (command.equalsIgnoreCase("M")) {
    // Mover N grados a una velocidad específica
    stopMotor(); // Detener movimiento continuo si está activo
    int commaIndex = input.indexOf(',');
    if (commaIndex != -1) {
      String degreesStr = input.substring(0, commaIndex);
      String rpmStr = input.substring(commaIndex + 1);
      float degrees = degreesStr.toFloat();
      float rpm = rpmStr.toFloat();
      if (degrees != 0 && rpm != 0) {
        Serial.print("Moviendo ");
        Serial.print(degrees);
        Serial.print(" grados a ");
        Serial.print(rpm);
        Serial.println(" RPM.");
        moveDegrees(degrees, rpm);
        Serial.println("Movimiento completado.");
      } else {
        Serial.println("Error: Valores inválidos.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use M<grados>,<rpm>");
    }
  } else if (command.equalsIgnoreCase("S")) {
    // Establecer velocidad continua
    float rpm = input.toFloat();
    if (rpm != 0) {
      Serial.print("Estableciendo velocidad continua a ");
      Serial.print(rpm);
      Serial.println(" RPM.");
      setSpeed(rpm);
    } else {
      Serial.println("Error: Valor de RPM inválido.");
    }
  } else if (command.equalsIgnoreCase("P")) {
    // Detener el motor
    stopMotor();
    Serial.println("Motor detenido.");
  } else if (command.equalsIgnoreCase("AL") || command.equalsIgnoreCase("AS")) {
    // Aceleración lineal o curva S
    stopMotor(); // Detener cualquier movimiento continuo
    int firstCommaIndex = input.indexOf(',');
    int secondCommaIndex = input.indexOf(',', firstCommaIndex + 1);
    if (firstCommaIndex != -1 && secondCommaIndex != -1) {
      String initialRPMStr = input.substring(0, firstCommaIndex);
      String finalRPMStr = input.substring(firstCommaIndex + 1, secondCommaIndex);
      String timeStr = input.substring(secondCommaIndex + 1);
      float initRPM = initialRPMStr.toFloat();
      float finRPM = finalRPMStr.toFloat();
      float timeSec = timeStr.toFloat();
      if (timeSec > 0) {
        Serial.print("Acelerando de ");
        Serial.print(initRPM);
        Serial.print(" RPM a ");
        Serial.print(finRPM);
        Serial.print(" RPM en ");
        Serial.print(timeSec);
        Serial.print(" segundos usando aceleración ");
        Serial.println(command.equalsIgnoreCase("AL") ? "lineal." : "curva S.");
        accelerate(initRPM, finRPM, timeSec, command.equalsIgnoreCase("AL") ? "linear" : "s-curve");
      } else {
        Serial.println("Error: Tiempo inválido.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use AL<rpmInicial>,<rpmFinal>,<tiempoSegundos> o AS<rpmInicial>,<rpmFinal>,<tiempoSegundos>");
    }
  } else {
    Serial.println("Comando no reconocido.");
  }
}
