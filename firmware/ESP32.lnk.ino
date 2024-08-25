#include <WiFi.h>
#include <WebServer.h>

// Configuración del punto de acceso (AP)
const char* ssid = "ESP32_AP";
const char* password = "123456789";

// Dirección IP fija para el AP
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Creación del servidor web en el puerto 80
WebServer server(80);

// Variables globales
float velocidad_disco = 0.0;
float velocidad_bolita = 0.0;
float torque = 2.5;  // Valor inicial del torque

// Función para manejar la solicitud de establecer la velocidad del disco
void handleSetDisco() {
  if (server.hasArg("rpm")) {
    velocidad_disco = server.arg("rpm").toFloat();
    Serial.println("Velocidad del disco ajustada: " + String(velocidad_disco));
    server.send(200, "text/plain", "Velocidad del disco ajustada");
  } else {
    server.send(400, "text/plain", "Falta el parámetro rpm");
  }
}

// Función para manejar la solicitud de establecer la velocidad de la bolita
void handleSetBolita() {
  if (server.hasArg("rpm")) {
    velocidad_bolita = server.arg("rpm").toFloat();
    Serial.println("Velocidad de la bolita ajustada: " + String(velocidad_bolita));
    server.send(200, "text/plain", "Velocidad de la bolita ajustada");
  } else {
    server.send(400, "text/plain", "Falta el parámetro rpm");
  }
}

// Función para manejar la solicitud de obtener el torque
void handleGetTorque() {
  Serial.println("Solicitud de torque recibida.");
  server.send(200, "text/plain", String(torque));
}

// Función para manejar la solicitud de establecer el torque
void handleSetTorque() {
  if (server.hasArg("value")) {
    torque = server.arg("value").toFloat();
    Serial.println("Torque ajustado: " + String(torque));
    server.send(200, "text/plain", "Torque ajustado");
  } else {
    server.send(400, "text/plain", "Falta el parámetro value");
  }
}

void setup() {
  Serial.begin(115200);

  // Configuración del AP
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  Serial.println("Punto de acceso iniciado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.softAPIP());

  // Configuración de las rutas HTTP
  server.on("/set_disco", handleSetDisco);
  server.on("/set_bolita", handleSetBolita);
  server.on("/get_torque", handleGetTorque);
  server.on("/set_torque", handleSetTorque);

  // Iniciar servidor
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();
}
