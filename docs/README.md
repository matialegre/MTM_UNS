### Proyecto: Sistema de Control de Torque y Velocidad con ESP32

Este proyecto aborda el diseño y la implementación de un sistema avanzado de control para dos motores paso a paso NEMA 17 utilizando un microcontrolador ESP32 y un sensor de torque FYAH. La solución incluye una interfaz gráfica desarrollada en Python, que permite el monitoreo y ajuste en tiempo real de los parámetros del sistema, facilitando la experimentación y análisis en aplicaciones de ingeniería.

## Estructura del Proyecto

El proyecto está organizado en directorios que facilitan la modularización del desarrollo, permitiendo un manejo eficiente tanto del código como de la documentación y pruebas. A continuación, se describe en detalle cada componente:

### /documentación
Contiene toda la información relevante para la comprensión, instalación y uso del sistema:
- **README.md:** Proporciona una visión general del proyecto, detallando su estructura, requisitos de hardware y software, y las características clave.
- **especificaciones_del_sistema.md:** Describe las especificaciones técnicas, incluyendo la descripción del hardware (ESP32, sensores, actuadores) y el software utilizado.
- **guía_de_instalación.md:** Instrucciones paso a paso para configurar el entorno de desarrollo y ejecutar el sistema.
- **CAMBIOS.md:** Registro histórico de las versiones del sistema y sus respectivas mejoras.

### /firmware
Contiene el código fuente que se ejecuta en el ESP32, dividido en módulos para mejorar la mantenibilidad y escalabilidad:
- **principal.cpp:** Punto de entrada del firmware. Inicializa el sistema y coordina la comunicación entre los diferentes componentes.
- **/lib:** Bibliotecas que encapsulan la lógica de control de hardware.
  - **controlador_motor.cpp/h:** Controla la operación de los motores NEMA 17 utilizando el protocolo PWM.
  - **manejador_sensor.cpp/h:** Gestión del sensor de torque FYAH, incluyendo la lectura y calibración del mismo.
- **/include:** Archivos de cabecera para las bibliotecas mencionadas.
- **/src:** Código adicional, como la gestión de conectividad WiFi mediante el módulo `gestor_wifi.cpp`.

### /gui
Contiene la interfaz gráfica desarrollada en Python, responsable de la interacción del usuario con el sistema:
- **principal.py:** Archivo principal de la GUI, desde donde se lanza la aplicación.
- **/vistas:** Define las ventanas y componentes gráficos.
  - **ventana_principal.py:** Contiene la lógica de la ventana principal, donde se visualizan los datos y se ajustan los parámetros.
- **/controladores:** Controladores que manejan la lógica entre la interfaz gráfica y el firmware del ESP32.
  - **controlador_motor.py:** Se encarga de enviar comandos al ESP32 y recibir datos de torque.
- **/utilidades:** Herramientas auxiliares para la gestión de configuraciones.
- **/recursos:** Contiene recursos gráficos como íconos e imágenes.

### /pruebas
Incluye scripts diseñados para validar la funcionalidad del sistema mediante pruebas unitarias e integradas:
- **prueba_control_motor.py:** Verifica la correcta operación del controlador del motor.
- **prueba_conexión_wifi.py:** Evalúa la conectividad WiFi del ESP32.
- **/pruebas_de_integración:** Asegura la cohesión entre diferentes módulos del sistema.
- **/pruebas_unitarias:** Pruebas detalladas de funciones específicas.

### /calibración
Almacena los scripts y datos necesarios para calibrar el sensor de torque:
- **datos_calibración.csv:** Datos experimentales utilizados para ajustar la curva de calibración del sensor.
- **script_calibración.py:** Script que realiza el ajuste de la curva de calibración y genera los coeficientes correspondientes.
- **notas_calibración.md:** Documenta el proceso de calibración, incluyendo observaciones y resultados.

## Requisitos Técnicos

### Hardware

#### Microcontrolador: ESP32
El ESP32 es un microcontrolador de alta eficiencia que incluye conectividad WiFi y Bluetooth, con capacidad de operar a 240 MHz y con una memoria flash de 4 MB. Este microcontrolador es ideal para aplicaciones de control debido a su capacidad para manejar múltiples periféricos y su soporte para FreeRTOS, permitiendo una programación concurrente y eficiente.

#### Sensores: FYAH (0-5Nm)
El sensor FYAH es un dispositivo de precisión diseñado para medir el torque en un rango de 0 a 5 Nm. La señal analógica de salida (0-5V) es capturada por el ESP32 a través de un conversor ADC, que la convierte en una lectura digital para su posterior procesamiento.

#### Actuadores: 2x Motor NEMA 17, 2x Driver TB6600
Los motores NEMA 17 son motores paso a paso de alta precisión con un torque nominal de 45 Ncm. Son controlados mediante los drivers TB6600, que permiten la modulación de la velocidad y dirección de los motores mediante señales PWM generadas por el ESP32.

#### Fuentes de Alimentación
- **12V/5A**: Alimenta los motores NEMA 17.
- **5V/2A**: Alimenta el ESP32, garantizando un suministro estable para su operación.

### Software

#### ESP-IDF (ESP32 IoT Development Framework)
El firmware está desarrollado en C/C++ usando ESP-IDF, un entorno de desarrollo proporcionado por Espressif que permite el control directo del hardware del ESP32, así como la gestión de conectividad WiFi y Bluetooth.

#### Python 3.x con Tkinter y Matplotlib
La interfaz gráfica está construida en Python utilizando Tkinter para la GUI y Matplotlib para la representación gráfica de datos. PySerial se utiliza para la comunicación serial entre el ESP32 y la aplicación en Python.

## Fundamentos Teóricos

### Control de Motores Paso a Paso
Los motores paso a paso, como los NEMA 17 utilizados en este proyecto, son actuadores que permiten un control preciso de la posición y la velocidad mediante la aplicación de pulsos de corriente en sus bobinas. El ESP32 genera las señales PWM necesarias para controlar la velocidad y dirección de los motores a través de los drivers TB6600.

### Medición de Torque
El torque se mide usando el sensor FYAH, que convierte la fuerza aplicada en una señal de voltaje proporcional. Este voltaje es digitalizado por el ESP32 para ser utilizado en los cálculos de control. La calibración del sensor es crítica para asegurar que las lecturas sean precisas y lineales.

### Hidrodinámica y Lubricación Elasto-Hidrodinámica
El estudio de la lubricación elasto-hidrodinámica (EHL) es relevante en sistemas donde la fricción y el desgaste son factores clave. En este proyecto, se podría considerar la implementación de algoritmos de control que minimicen estos efectos, basándose en la teoría de la lubricación.

#### Ecuación de Reynolds
La ecuación de Reynolds, fundamental en la teoría de lubricación, describe la presión en una película lubricante bajo flujo laminar en un espacio estrecho entre dos superficies:

\[ \frac{\partial}{\partial x} \left( h^3 \frac{\partial p}{\partial x} \right) + \frac{\partial}{\partial y} \left( h^3 \frac{\partial p}{\partial y} \right) = 6 \eta U \frac{\partial h}{\partial x} \]

Donde:
- \( h \) es el espesor de la película lubricante.
- \( p \) es la presión en la película.
- \( \eta \) es la viscosidad del lubricante.
- \( U \) es la velocidad de deslizamiento entre las superficies.

Esta ecuación puede extenderse para modelar sistemas donde la lubricación es un factor crucial, proporcionando un marco teórico para el análisis de la fricción y el desgaste.

## Características del Sistema

1. **Control de Velocidad:** El sistema permite ajustar la velocidad de los motores NEMA 17 en tiempo real, con un motor configurado para variar su velocidad y otro para operar a una velocidad fija.
2. **Medición y Monitoreo del Torque:** El sensor FYAH proporciona datos en tiempo real del torque aplicado, permitiendo su visualización y análisis mediante la interfaz gráfica.
3. **Interfaz Gráfica Intuitiva:** La GUI permite al usuario ajustar parámetros del sistema y visualizar los resultados en gráficos de fácil interpretación.
4. **Comunicación Robustez:** La comunicación entre el ESP32 y la PC se realiza mediante UART, con un manejo de errores implementado para garantizar la integridad de los datos.

Este proyecto combina el control preciso de motores, la medición de torque, y una interfaz gráfica avanzada para proporcionar una plataforma ideal para aplicaciones de experimentación y análisis en ingeniería, alineándose con los estándares académicos y de investigación en universidades como la UNS de Bahía Blanca.
