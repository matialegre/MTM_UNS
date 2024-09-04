#include "HX711.h"

// Definir los pines de conexión
const int DT_PIN = 4; // Pin de datos (DT)
const int SCK_PIN = 5; // Pin de reloj (SCK)

// Crear una instancia del HX711
HX711 scale;

void setup() {
  // Inicializar la comunicación serie
  Serial.begin(115200);
  delay(500);

  // Configurar el HX711
  scale.begin(DT_PIN, SCK_PIN);

  // Calibración inicial (ajusta este valor según tu configuración)
  scale.set_scale(2280.f); // Establece la escala (ajusta este valor según tu sensor)
  scale.tare();            // Calibra el peso vacío

  Serial.println("HX711 listo");
}

void loop() {
  // Leer los valores del sensor de carga
  float weight = scale.get_units(10); // Promedio de 10 lecturas para mayor precisión

  // Mostrar el peso en el monitor serie
  Serial.print("Peso: ");
  Serial.print(weight, 2); // Muestra con dos decimales
  Serial.println(" gramos");

  delay(500); // Retraso para la siguiente lectura
}
