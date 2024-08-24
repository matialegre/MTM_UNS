# Sistema de Control de Torque y Velocidad con ESP32

Este proyecto implementa un sistema avanzado de control de dos motores NEMA 17 utilizando un ESP32 y un sensor de torque FYAH. La interfaz gráfica, desarrollada en Python, permite el monitoreo y ajuste en tiempo real de los parámetros del sistema.

## Estructura del Proyecto

- **/documentación**: Contiene toda la documentación relevante del proyecto.
- **/firmware**: Código fuente del firmware para el ESP32, incluyendo controladores y lógica de comunicación.
- **/gui**: Código fuente de la interfaz gráfica desarrollada en Python.
- **/pruebas**: Scripts y archivos para pruebas unitarias e integradas.
- **/calibración**: Scripts y datos para la calibración del sensor de torque.

## Requisitos

### Hardware

- ESP32
- Sensores: FYAH (0-5Nm)
- Actuadores: 2x Motor NEMA 17, 2x Driver TB6600
- Fuentes de alimentación: 12V/5A para motores, 5V/2A para ESP32

### Software

- ESP-IDF (para el firmware del ESP32)
- Python 3.x con Tkinter y Matplotlib

## Características

- Control de velocidad de dos motores NEMA 17 (uno con velocidad variable, otro fijo).
- Medición y monitoreo de torque en tiempo real.
- Interfaz gráfica para ajuste de parámetros y visualización de datos.
- Comunicación robusta entre ESP32 y PC usando UART.
