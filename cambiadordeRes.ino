#include <Arduino.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

// Definición de pines para el primer motor
#define STEP_PIN1 16
#define DIR_PIN1 17

// Definición de pines para el segundo motor
#define STEP_PIN2 18
#define DIR_PIN2 19

// Pin del sensor de torque (celda de carga)
#define TORQUE_PIN 34 // Pin ADC para leer el torque

// Parámetros del motor y driver
int microstepsPerRevolution = 6400; // Se puede actualizar desde la interfaz

// Variables para almacenar el torque leído
float torque_actual = 0;

// Variables para envío de datos al PC
unsigned long lastDataSentTime = 0;

// Variables para control de motores
volatile bool motor1_running = false;
volatile bool motor2_running = false;

// Variables para aceleración de motores
float targetRPM1 = 0;
float targetRPM2 = 0;
float currentRPM1 = 0;
float currentRPM2 = 0;
float acceleration1 = 0;
float acceleration2 = 0;
unsigned long lastUpdateTime1 = 0;
unsigned long lastUpdateTime2 = 0;
unsigned long accelerationStartTime1 = 0;
unsigned long accelerationStartTime2 = 0;
String accelerationMode1 = ""; // "linear" o "s-curve"
String accelerationMode2 = "";

// Variables para manejo de entrada serial
String inputString = "";
bool inputComplete = false;

void setup() {
  // Configuración de pines de dirección
  pinMode(DIR_PIN1, OUTPUT);
  pinMode(DIR_PIN2, OUTPUT);

  // Configuración del pin del sensor de torque
  pinMode(TORQUE_PIN, INPUT);

  // Configuración del puerto serie
  Serial.begin(115200);
  Serial.println("Control de 2 Motores Paso a Paso Iniciado con MCPWM y Lectura de Torque.");

  // Reservar memoria para la cadena de entrada
  inputString.reserve(200);

  // Configuración de MCPWM para los motores
  setupMCPWM();

  // Detener los motores al inicio
  stopMotor(1);
  stopMotor(2);
}

void loop() {
  // Lectura del torque
  readTorque();

  // Enviar datos al PC cada 100 ms
  if (millis() - lastDataSentTime >= 100) {
    sendDataToPC();
    lastDataSentTime = millis();
  }

  // Actualizar velocidad de motores según aceleración
  updateMotorSpeed(1);
  updateMotorSpeed(2);

  // Verificar si hay datos disponibles en el puerto serie
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputString.length() > 0) {
        inputComplete = true;
      }
    } else {
      inputString += inChar;
    }
  }

  if (inputComplete) {
    parseSerialInput();
    inputString = "";
    inputComplete = false;
  }
}

// Configuración inicial de MCPWM
void setupMCPWM() {
  // Inicializar MCPWM unidad 0 para el Motor 1
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, STEP_PIN1);

  mcpwm_config_t pwm_config1;
  pwm_config1.frequency = 1;  // Frecuencia inicial en Hz
  pwm_config1.cmpr_a = 50.0;  // Ciclo de trabajo inicial del 50%
  pwm_config1.cmpr_b = 0.0;
  pwm_config1.counter_mode = MCPWM_UP_COUNTER;
  pwm_config1.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config1);

  // Inicializar MCPWM unidad 1 para el Motor 2
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, STEP_PIN2);

  mcpwm_config_t pwm_config2;
  pwm_config2.frequency = 1;  // Frecuencia inicial en Hz
  pwm_config2.cmpr_a = 50.0;  // Ciclo de trabajo inicial del 50%
  pwm_config2.cmpr_b = 0.0;
  pwm_config2.counter_mode = MCPWM_UP_COUNTER;
  pwm_config2.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config2);
}

// Función para iniciar el motor usando MCPWM
void startMotor(int motorNumber, float rpm) {
  float frequency = (rpm * microstepsPerRevolution) / 60.0; // Frecuencia en Hz
  if (frequency <= 0) frequency = 1;

  if (motorNumber == 1) {
    mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, frequency);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0); // 50% de duty cycle
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    motor1_running = true;
    currentRPM1 = rpm;
  } else {
    mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_0, frequency);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0); // 50% de duty cycle
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    motor2_running = true;
    currentRPM2 = rpm;
  }
}

// Función para detener un motor
void stopMotor(int motorNumber) {
  if (motorNumber == 1) {
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
    motor1_running = false;
    currentRPM1 = 0;
    acceleration1 = 0; // Detener aceleración
  } else {
    mcpwm_set_signal_low(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A);
    motor2_running = false;
    currentRPM2 = 0;
    acceleration2 = 0; // Detener aceleración
  }
}

// Función para actualizar la velocidad del motor según la aceleración
void updateMotorSpeed(int motorNumber) {
  unsigned long currentTime = millis();
  float *currentRPM;
  float *targetRPM;
  float *acceleration;
  unsigned long *lastUpdateTime;
  unsigned long *accelerationStartTime;
  String *accelerationMode;

  if (motorNumber == 1) {
    currentRPM = &currentRPM1;
    targetRPM = &targetRPM1;
    acceleration = &acceleration1;
    lastUpdateTime = &lastUpdateTime1;
    accelerationStartTime = &accelerationStartTime1;
    accelerationMode = &accelerationMode1;
  } else {
    currentRPM = &currentRPM2;
    targetRPM = &targetRPM2;
    acceleration = &acceleration2;
    lastUpdateTime = &lastUpdateTime2;
    accelerationStartTime = &accelerationStartTime2;
    accelerationMode = &accelerationMode2;
  }

  if (*acceleration > 0 && *currentRPM != *targetRPM) {
    float deltaTime = (currentTime - *lastUpdateTime) / 1000.0; // Convertir a segundos

    float rpmChange = (*acceleration) * deltaTime;

    if (*accelerationMode == "linear") {
      if (*currentRPM < *targetRPM) {
        *currentRPM += rpmChange;
        if (*currentRPM > *targetRPM) *currentRPM = *targetRPM;
      } else {
        *currentRPM -= rpmChange;
        if (*currentRPM < *targetRPM) *currentRPM = *targetRPM;
      }
    } else if (*accelerationMode == "s-curve") {
      // Implementación simplificada de curva S
      float totalTime = abs(*targetRPM - *currentRPM) / *acceleration; // Tiempo total de aceleración en segundos
      float elapsedTime = (currentTime - *accelerationStartTime) / 1000.0; // Tiempo desde que comenzó la aceleración
      float progress = elapsedTime / totalTime;
      if (progress > 1.0) progress = 1.0;
      float sCurveFactor = (1 - cos(PI * progress)) / 2;
      *currentRPM = *currentRPM + ((*targetRPM - *currentRPM) * sCurveFactor);
    }

    // Actualizar la frecuencia del MCPWM
    adjustMotorFrequency(motorNumber, *currentRPM);

    // Actualizar lastUpdateTime
    *lastUpdateTime = currentTime;

    // Si alcanzó la RPM objetivo, detener aceleración
    if (*currentRPM == *targetRPM) {
      *acceleration = 0;
    }
  }
}

// Función para ajustar la frecuencia del motor
void adjustMotorFrequency(int motorNumber, float rpm) {
  float frequency = (rpm * microstepsPerRevolution) / 60.0; // Frecuencia en Hz
  if (frequency <= 0) frequency = 1;

  if (motorNumber == 1) {
    mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, frequency);
  } else {
    mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_0, frequency);
  }
}

// Función para hacer girar un motor un número específico de grados a una RPM determinada
void rotateDegrees(int motorNumber, float degrees, float rpm) {
  int dir_pin = (motorNumber == 1) ? DIR_PIN1 : DIR_PIN2;

  // Establecer dirección (ajusta según tu necesidad)
  digitalWrite(dir_pin, HIGH); // o LOW según el sentido deseado

  float frequency = (rpm * microstepsPerRevolution) / 60.0; // Frecuencia en Hz
  if (frequency <= 0) frequency = 1;

  // Calcular el número de pasos necesarios
  int steps = (degrees / 360.0) * microstepsPerRevolution;

  // Iniciar el motor
  startMotor(motorNumber, rpm);

  // Calcular el tiempo total necesario
  float timePerStep = 1.0 / frequency;
  float totalTime = steps * timePerStep * 1000.0; // En milisegundos

  // Esperar el tiempo necesario
  delay(totalTime);

  // Detener el motor
  stopMotor(motorNumber);
}

// Función para leer el torque
void readTorque() {
  int rawValue = analogRead(TORQUE_PIN);
  // Convertir el valor analógico a torque real (ajusta según tu sensor)
  float voltage = (rawValue / 4095.0) * 3.3; // Para ADC de 12 bits y 3.3V
  torque_actual = (voltage / 3.3) * 5.0; // Supongamos que 3.3V corresponde a 5 Nm
}

// Función para enviar datos al PC
void sendDataToPC() {
  String dataString = "Torque:";
  dataString += torque_actual;
  dataString += ",RPM1:";
  dataString += currentRPM1;
  dataString += ",RPM2:";
  dataString += currentRPM2;
  Serial.println(dataString);
}

// Función para analizar la entrada del usuario
void parseSerialInput() {
  String input = inputString;
  input.trim();

  if (input.length() == 0) return;

  // Comando para configurar microstepsPerRevolution
  if (input.startsWith("MSPR")) {
    int newMicrosteps = input.substring(4).toInt();
    if (newMicrosteps > 0) {
      microstepsPerRevolution = newMicrosteps;
      Serial.print("microstepsPerRevolution actualizado a ");
      Serial.println(microstepsPerRevolution);
    } else {
      Serial.println("Error: Valor de microstepsPerRevolution inválido.");
    }
  }

  // Comandos para Motor 1
  else if (input.startsWith("S1")) {
    // Establecer velocidad continua para Motor 1
    float rpm = input.substring(2).toFloat();
    Serial.print("Estableciendo velocidad continua en el Motor 1 a ");
    Serial.print(rpm);
    Serial.println(" RPM.");
    targetRPM1 = rpm;
    acceleration1 = 0; // Sin aceleración
    currentRPM1 = rpm;
    accelerationMode1 = "linear";
    accelerationStartTime1 = millis();
    lastUpdateTime1 = millis();
    startMotor(1, currentRPM1);
  } else if (input.startsWith("P1")) {
    // Detener Motor 1
    stopMotor(1);
    Serial.println("Motor 1 detenido.");
  } else if (input.startsWith("AL1") || input.startsWith("AS1")) {
    // Aceleración lineal o curva S para Motor 1
    int firstCommaIndex = input.indexOf(',');
    int secondCommaIndex = input.indexOf(',', firstCommaIndex + 1);
    if (firstCommaIndex != -1 && secondCommaIndex != -1) {
      String initialRPMStr = input.substring(3, firstCommaIndex);
      String finalRPMStr = input.substring(firstCommaIndex + 1, secondCommaIndex);
      String timeStr = input.substring(secondCommaIndex + 1);
      float initRPM = initialRPMStr.toFloat();
      float finRPM = finalRPMStr.toFloat();
      float timeSec = timeStr.toFloat();
      if (timeSec > 0) {
        Serial.print("Acelerando el Motor 1 de ");
        Serial.print(initRPM);
        Serial.print(" RPM a ");
        Serial.print(finRPM);
        Serial.print(" RPM en ");
        Serial.print(timeSec);
        Serial.print(" segundos usando aceleración ");
        Serial.println(input.startsWith("AL1") ? "lineal." : "curva S.");
        targetRPM1 = finRPM;
        acceleration1 = abs(finRPM - initRPM) / timeSec;
        currentRPM1 = initRPM;
        accelerationMode1 = input.startsWith("AL1") ? "linear" : "s-curve";
        accelerationStartTime1 = millis();
        lastUpdateTime1 = millis();
        startMotor(1, currentRPM1);
      } else {
        Serial.println("Error: Tiempo inválido.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use AL1<rpmInicial>,<rpmFinal>,<tiempoSegundos> o AS1<rpmInicial>,<rpmFinal>,<tiempoSegundos>");
    }
  } else if (input.startsWith("M1")) {
    // Mover Motor 1 N grados a una velocidad específica
    int commaIndex = input.indexOf(',');
    if (commaIndex != -1) {
      String degreesStr = input.substring(2, commaIndex);
      String rpmStr = input.substring(commaIndex + 1);
      float degrees = degreesStr.toFloat();
      float rpm = rpmStr.toFloat();
      if (degrees != 0 && rpm != 0) {
        Serial.print("Moviendo Motor 1 ");
        Serial.print(degrees);
        Serial.print(" grados a ");
        Serial.print(rpm);
        Serial.println(" RPM.");
        rotateDegrees(1, degrees, rpm);
      } else {
        Serial.println("Error: Valores inválidos.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use M1<grados>,<rpm>");
    }
  }

  // Comandos para Motor 2
  else if (input.startsWith("S2")) {
    // Establecer velocidad continua para Motor 2
    float rpm = input.substring(2).toFloat();
    Serial.print("Estableciendo velocidad continua en el Motor 2 a ");
    Serial.print(rpm);
    Serial.println(" RPM.");
    targetRPM2 = rpm;
    acceleration2 = 0; // Sin aceleración
    currentRPM2 = rpm;
    accelerationMode2 = "linear";
    accelerationStartTime2 = millis();
    lastUpdateTime2 = millis();
    startMotor(2, currentRPM2);
  } else if (input.startsWith("P2")) {
    // Detener Motor 2
    stopMotor(2);
    Serial.println("Motor 2 detenido.");
  } else if (input.startsWith("AL2") || input.startsWith("AS2")) {
    // Aceleración lineal o curva S para Motor 2
    int firstCommaIndex = input.indexOf(',');
    int secondCommaIndex = input.indexOf(',', firstCommaIndex + 1);
    if (firstCommaIndex != -1 && secondCommaIndex != -1) {
      String initialRPMStr = input.substring(3, firstCommaIndex);
      String finalRPMStr = input.substring(firstCommaIndex + 1, secondCommaIndex);
      String timeStr = input.substring(secondCommaIndex + 1);
      float initRPM = initialRPMStr.toFloat();
      float finRPM = finalRPMStr.toFloat();
      float timeSec = timeStr.toFloat();
      if (timeSec > 0) {
        Serial.print("Acelerando el Motor 2 de ");
        Serial.print(initRPM);
        Serial.print(" RPM a ");
        Serial.print(finRPM);
        Serial.print(" RPM en ");
        Serial.print(timeSec);
        Serial.print(" segundos usando aceleración ");
        Serial.println(input.startsWith("AL2") ? "lineal." : "curva S.");
        targetRPM2 = finRPM;
        acceleration2 = abs(finRPM - initRPM) / timeSec;
        currentRPM2 = initRPM;
        accelerationMode2 = input.startsWith("AL2") ? "linear" : "s-curve";
        accelerationStartTime2 = millis();
        lastUpdateTime2 = millis();
        startMotor(2, currentRPM2);
      } else {
        Serial.println("Error: Tiempo inválido.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use AL2<rpmInicial>,<rpmFinal>,<tiempoSegundos> o AS2<rpmInicial>,<rpmFinal>,<tiempoSegundos>");
    }
  } else if (input.startsWith("M2")) {
    // Mover Motor 2 N grados a una velocidad específica
    int commaIndex = input.indexOf(',');
    if (commaIndex != -1) {
      String degreesStr = input.substring(2, commaIndex);
      String rpmStr = input.substring(commaIndex + 1);
      float degrees = degreesStr.toFloat();
      float rpm = rpmStr.toFloat();
      if (degrees != 0 && rpm != 0) {
        Serial.print("Moviendo Motor 2 ");
        Serial.print(degrees);
        Serial.print(" grados a ");
        Serial.print(rpm);
        Serial.println(" RPM.");
        rotateDegrees(2, degrees, rpm);
      } else {
        Serial.println("Error: Valores inválidos.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Use M2<grados>,<rpm>");
    }
  } else {
    Serial.println("Comando no reconocido.");
  }
}
