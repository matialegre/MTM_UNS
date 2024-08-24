# Especificaciones del Sistema

## Hardware

### Microcontrolador: ESP32
- Frecuencia: 240 MHz
- Memoria Flash: 4 MB
- WiFi/Bluetooth: Integrado

### Sensores: FYAH
- Rango de Torque: 0-5 Nm
- Salida: Analógica (0-5V)

### Actuadores
- Motores: 2x NEMA 17 (45 Ncm)
- Drivers: 2x TB6600
- Control: Señales STEP/DIR desde el ESP32

### Fuentes de Alimentación
- 12V/5A para motores
- 5V/2A para ESP32
https://www.youtube.com/
## Software

### Firmware ESP32
- Programado en C/C++ usando ESP-IDF
- Control de motores usando PWM
- Medición de torque
- Comunicación UART

### Interfaz Python
- Desarrollada en Python 3.x
- Tkinter para GUI
- Matplotlib para gráficos
- PySerial para UART

## Funcionalidades Clave
- Control de velocidad en tiempo real
- Monitoreo y visualización de torque
- Manejo de errores y robustez en comunicación
