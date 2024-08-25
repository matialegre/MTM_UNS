# Guía de Instalación

## Requisitos

### Hardware
- ESP32 configurado para programación con ESP-IDF
- Sensores y Actuadores conectados al ESP32

### Software
- ESP-IDF instalado
- Python 3.x con Tkinter, Matplotlib y PySerial

## Instalación

1. **Clonar el Repositorio:**
   ```bash
   git clone https://github.com/tu-repositorio/proyecto-control-torque.git
   cd proyecto-control-torque
2.Configuración del Firmware:
   cd firmware
   idf.py set-target esp32
   idf.py menuconfig
   idf.py build
   idf.py flash
   idf.py monitor

3.Instalación de Dependencias en Python:
   pip install -r requirements.txt
   
4. Ejecución de la GUI:
   cd gui
   python principal.py

5. Calibración Inicial: Ejecuta script_calibración.py para calibrar el sensor de torque usando los datos en        datos_calibración.csv.


```markdown
# Registro de Cambios

## [v1.0.1] - 2024-09-01

### Añadido
- Modularización completa del código en controladores de hardware y lógica de negocio.
- Manejo robusto de errores en UART.
- Optimización del uso de memoria en el ESP32.
- Interfaz gráfica mejorada con soporte para guardado/carga de configuraciones.
