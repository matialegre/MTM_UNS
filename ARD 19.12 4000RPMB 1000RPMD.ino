#include <WiFi.h>
#include <WebServer.h>

// Configuración de red Wi-Fi
const char *ssid = "Controlador_ESP32";
const char *password = "12345678";

// Pines GPIO
#define PIN_D4 4  
#define PIN_D18 18  
#define PIN_D13 13  
#define PIN_D14 14  
#define PIN_D27 27  
#define PIN_D26 26

// Servidor web
WebServer server(80);

// Funciones para controlar los pines
void encender() { // Circuito 1
  digitalWrite(PIN_D4, HIGH);
  digitalWrite(PIN_D18, LOW);
  
  digitalWrite(PIN_D13, LOW);
  digitalWrite(PIN_D14, LOW);
  digitalWrite(PIN_D27, HIGH);
  digitalWrite(PIN_D26, HIGH);
  server.send(200, "text/plain", "Circuito 1 activado");
}

void apagar() { // Circuito 2
  digitalWrite(PIN_D4, LOW);
  digitalWrite(PIN_D18, HIGH);
  
  digitalWrite(PIN_D13, HIGH);
  digitalWrite(PIN_D14, HIGH);
  digitalWrite(PIN_D27, LOW);
  digitalWrite(PIN_D26, LOW);
  server.send(200, "text/plain", "Circuito 2 activado");
}

void estadoEspecial() { // Apagado
  digitalWrite(PIN_D4, HIGH);
  digitalWrite(PIN_D18, HIGH);
  digitalWrite(PIN_D13, HIGH);
  digitalWrite(PIN_D14, HIGH);
  digitalWrite(PIN_D27, HIGH);
  digitalWrite(PIN_D26, HIGH);
  server.send(200, "text/plain", "Apagado");
}

// Página principal con interfaz
void handleRoot() {
  String html = "<!DOCTYPE html>\
                <html>\
                <head>\
                  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\
                  <style>\
                    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\
                    h1 { color: #333; }\
                    button {\
                      font-size: 18px;\
                      padding: 10px 20px;\
                      margin: 10px;\
                      color: white;\
                      background-color: #007BFF;\
                      border: none;\
                      border-radius: 5px;\
                      cursor: pointer;\
                    }\
                    button:hover { background-color: #0056b3; }\
                    .status { margin-top: 20px; font-size: 18px; color: #555; }\
                  </style>\
                </head>\
                <body>\
                  <h1>Control de Pines ESP32</h1>\
                  <button onclick=\"fetch('/encender').then(() => updateStatus('Circuito 1 activado'))\">Encender Circuito 1</button>\
                  <button onclick=\"fetch('/apagar').then(() => updateStatus('Circuito 2 activado'))\">Encender Circuito 2</button>\
                  <button onclick=\"fetch('/estado_especial').then(() => updateStatus('Apagado'))\">Apagado</button>\
                  <div class='status' id='status'>Estado: Apagado (por defecto)</div>\
                  <script>\
                    function updateStatus(status) {\
                      document.getElementById('status').innerText = 'Estado: ' + status;\
                    }\
                  </script>\
                </body>\
                </html>";
  server.send(200, "text/html", html);
}

void setup() {
  // Configuración de los pines como salida
  pinMode(PIN_D4, OUTPUT);
  pinMode(PIN_D18, OUTPUT);
  pinMode(PIN_D13, OUTPUT);
  pinMode(PIN_D14, OUTPUT);
  pinMode(PIN_D27, OUTPUT);
  pinMode(PIN_D26, OUTPUT);

  // Configurar Wi-Fi en modo AP
  WiFi.softAP(ssid, password);
  Serial.begin(115200);
  Serial.println("Punto de acceso iniciado");
  Serial.print("IP del AP: ");
  Serial.println(WiFi.softAPIP());

  // Configurar rutas del servidor
  server.on("/", handleRoot);
  server.on("/encender", encender);
  server.on("/apagar", apagar);
  server.on("/estado_especial", estadoEspecial);

  // Iniciar en estado apagado
  estadoEspecial();

  // Iniciar el servidor
  server.begin();
}

void loop() {
  // Manejar peticiones web
  server.handleClient();
}
