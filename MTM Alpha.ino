#include <Arduino.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include "HX711.h"  // Librería para la celda de carga

// Definiciones de pines GPIO
#define GPIO_DISCO_STEP    18
#define GPIO_DISCO_DIR     19
#define GPIO_BOLA_STEP     21
#define GPIO_BOLA_DIR      22

// Pines para la celda de carga
#define LOADCELL_DOUT_PIN  32
#define LOADCELL_SCK_PIN   33

// Canales LEDC
#define LEDC_CHANNEL_DISCO  LEDC_CHANNEL_0
#define LEDC_CHANNEL_BOLA   LEDC_CHANNEL_1

// Timer LEDC
#define LEDC_TIMER_DISCO    LEDC_TIMER_0
#define LEDC_TIMER_BOLA     LEDC_TIMER_1
#define LEDC_MODE           LEDC_HIGH_SPEED_MODE

// Parámetros iniciales
#define MICROSTEPPING      8
#define PASOS_POR_REV      (200 * MICROSTEPPING)
#define T_PULSE_MIN        5  // Ancho de pulso mínimo en us
#define T_LOW_MIN          2.5  // Tiempo bajo mínimo en us
#define DUTY_RESOLUTION    LEDC_TIMER_13_BIT  // Mayor resolución para mejor control del duty cycle

#define F_PWM_MAX          (1000000.0 / ((T_PULSE_MIN + T_LOW_MIN) * 2))  // Frecuencia máxima permitida
#define F_PWM_MIN          100  // Frecuencia mínima razonable (100 Hz)

// Variables globales
volatile float omega_D = 0.0;
volatile float omega_B = 0.0;
volatile float t_global = 0.0;
volatile float dt = 0.01;  // Intervalo de tiempo en segundos

// Parámetros del sistema (valores por defecto)
float RPM_D_i = 0.0;
float RPM_D_f = 300.0;
float R = 5.4;       // Radio del disco en cm
float r = 4.0;       // Radio de la bola en cm
float mu = 0.2;      // Coeficiente de deslizamiento (20%)
float T = 60.0;      // Tiempo total de aceleración en segundos

// Cálculos previos
float K = 1.0;
float omega_D_i = 0.0;
float omega_D_f = 0.0;
float a_D = 0.0;
float omega_B_i = 0.0;
float omega_B_f = 0.0;
float a_B = 0.0;

// Variables para control
bool aceleracion_activa = false;
unsigned long tiempo_anterior = 0;
unsigned long tiempo_serial = 0;

// Objeto para la celda de carga
HX711 scale;

void configurarCeldaCarga() {
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(2280.f);  // Ajusta esto con el factor de calibración adecuado para tu celda de carga
    scale.tare();             // Reinicia la escala a cero
}

void mostrarCalculosIniciales() {
    Serial.println("=== Cálculos Iniciales ===");
    Serial.print("Tiempo de aceleración: "); Serial.print(T); Serial.println(" s");
    Serial.print("Velocidad angular inicial Disco: "); Serial.print(omega_D_i); Serial.println(" rad/s");
    Serial.print("Velocidad angular final Disco: "); Serial.print(omega_D_f); Serial.println(" rad/s");
    Serial.print("Aceleración angular Disco: "); Serial.print(a_D); Serial.println(" rad/s^2");

    Serial.print("Velocidad angular inicial Bola: "); Serial.print(omega_B_i); Serial.println(" rad/s");
    Serial.print("Velocidad angular final Bola: "); Serial.print(omega_B_f); Serial.println(" rad/s");
    Serial.print("Aceleración angular Bola: "); Serial.print(a_B); Serial.println(" rad/s^2");

    // Calcular velocidades lineales máximas
    float v_D_max = omega_D_f * (R / 100.0);  // Convertir R a metros
    float v_B_max = omega_B_f * (r / 100.0);  // Convertir r a metros

    Serial.print("Velocidad lineal máxima Disco: "); Serial.print(v_D_max); Serial.println(" m/s");
    Serial.print("Velocidad lineal máxima Bola: "); Serial.print(v_B_max); Serial.println(" m/s");

    // Calcular frecuencias máximas de pulsos
    float f_pulsos_D_max = (PASOS_POR_REV / (2 * PI)) * omega_D_f;
    float f_pulsos_B_max = (PASOS_POR_REV / (2 * PI)) * omega_B_f;

    Serial.print("Frecuencia máxima de pulsos Disco: "); Serial.print(f_pulsos_D_max); Serial.println(" Hz");
    Serial.print("Frecuencia máxima de pulsos Bola: "); Serial.print(f_pulsos_B_max); Serial.println(" Hz");
    Serial.println("==========================");
}

void configurarLEDC() {
    // Configurar timer LEDC para el disco
    ledc_timer_config_t ledc_timer_disco = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = DUTY_RESOLUTION,
        .timer_num        = LEDC_TIMER_DISCO,
        .freq_hz          = 5000,  // Frecuencia inicial (se actualizará)
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_disco);

    // Configurar timer LEDC para la bola
    ledc_timer_config_t ledc_timer_bola = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = DUTY_RESOLUTION,
        .timer_num        = LEDC_TIMER_BOLA,
        .freq_hz          = 5000,  // Frecuencia inicial (se actualizará)
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_bola);

    // Configurar canal para el disco
    ledc_channel_config_t ledc_channel_disco = {
        .gpio_num       = GPIO_DISCO_STEP,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_DISCO,
        .timer_sel      = LEDC_TIMER_DISCO,
        .duty           = 0, // Duty cycle inicial
        .hpoint         = 0,
        .flags          = {
            .output_invert = 0
        }
    };
    ledc_channel_config(&ledc_channel_disco);

    // Configurar canal para la bola
    ledc_channel_config_t ledc_channel_bola = {
        .gpio_num       = GPIO_BOLA_STEP,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_BOLA,
        .timer_sel      = LEDC_TIMER_BOLA,
        .duty           = 0, // Duty cycle inicial
        .hpoint         = 0,
        .flags          = {
            .output_invert = 0
        }
    };
    ledc_channel_config(&ledc_channel_bola);
}

void actualizarFrecuencias() {
    // Incrementar el tiempo
    t_global += dt;

    // Calcular las velocidades angulares actuales
    float omega_D_temp = omega_D_i + a_D * t_global;
    float omega_B_temp = omega_B_i + a_B * t_global;

    // Limitar a las velocidades finales
    if (omega_D_temp > omega_D_f) omega_D_temp = omega_D_f;
    if (omega_B_temp > omega_B_f) omega_B_temp = omega_B_f;

    // Calcular frecuencias de pulsos
    float f_pulsos_D = (PASOS_POR_REV / (2 * PI)) * omega_D_temp;
    float f_pulsos_B = (PASOS_POR_REV / (2 * PI)) * omega_B_temp;

    // Asegurar que las frecuencias están dentro de los límites
    if (f_pulsos_D > F_PWM_MAX) f_pulsos_D = F_PWM_MAX;
    if (f_pulsos_D < F_PWM_MIN) f_pulsos_D = F_PWM_MIN;
    if (f_pulsos_B > F_PWM_MAX) f_pulsos_B = F_PWM_MAX;
    if (f_pulsos_B < F_PWM_MIN) f_pulsos_B = F_PWM_MIN;

    // Calcular duty cycle para cumplir con el ancho de pulso mínimo
    uint32_t duty_max = (1 << DUTY_RESOLUTION) - 1;

    // Calcular el duty cycle en base al ancho de pulso mínimo
    uint32_t duty_D = (T_PULSE_MIN * f_pulsos_D * duty_max) / 1000000.0;
    uint32_t duty_B = (T_PULSE_MIN * f_pulsos_B * duty_max) / 1000000.0;

    // Asegurar que el duty cycle no sea menor que 1
    if (duty_D < 1) duty_D = 1;
    if (duty_B < 1) duty_B = 1;

    // Asegurar que el duty cycle no exceda el máximo
    if (duty_D > duty_max) duty_D = duty_max;
    if (duty_B > duty_max) duty_B = duty_max;

    // Actualizar frecuencia y duty cycle del disco
    ledc_set_freq(LEDC_MODE, LEDC_TIMER_DISCO, f_pulsos_D);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DISCO, duty_D);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DISCO);

    // Actualizar frecuencia y duty cycle de la bola
    ledc_set_freq(LEDC_MODE, LEDC_TIMER_BOLA, f_pulsos_B);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_BOLA, duty_B);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_BOLA);

    // Leer la celda de carga
    float peso = scale.get_units(5);  // Lee el peso promedio de 5 lecturas

    // Calcular velocidades lineales
    float v_D = omega_D_temp * (R / 100.0);  // Convertir R de cm a m
    float v_B = omega_B_temp * (r / 100.0);  // Convertir r de cm a m

    // Calcular tiempos en alto
    float T_pulse_D = ((float)duty_D / duty_max) / f_pulsos_D;  // Tiempo en alto del pulso del Disco en segundos
    float T_pulse_B = ((float)duty_B / duty_max) / f_pulsos_B;  // Tiempo en alto del pulso de la Bola en segundos

    // Convertir a microsegundos para mostrar
    float T_pulse_D_us = T_pulse_D * 1e6;
    float T_pulse_B_us = T_pulse_B * 1e6;

    // Mostrar información cada segundo
    if ((millis() - tiempo_serial) >= 500) {
        tiempo_serial = millis();
        Serial.print("Tiempo: "); Serial.print(t_global, 2); Serial.print(" s");
        Serial.print(" | Freq Disco: "); Serial.print(f_pulsos_D, 2); Serial.print(" Hz");
        Serial.print(" | Duty Disco: "); Serial.print(((float)duty_D / duty_max) * 100, 2); Serial.print("%");
        Serial.print(" | Velocidad Disco: "); Serial.print(v_D, 2); Serial.print(" m/s");
        Serial.print(" | Freq Bola: "); Serial.print(f_pulsos_B, 2); Serial.print(" Hz");
        Serial.print(" | Duty Bola: "); Serial.print(((float)duty_B / duty_max) * 100, 2); Serial.print("%");
        Serial.print(" | Velocidad Bola: "); Serial.print(v_B, 2); Serial.print(" m/s");
        Serial.print(" | Torque: "); Serial.print(peso, 2); Serial.print(" kg");
        Serial.print(" | T_pulse_D: "); Serial.print(T_pulse_D_us, 2); Serial.print(" us");
        Serial.print(" | T_pulse_B: "); Serial.print(T_pulse_B_us, 2); Serial.println(" us");
    }
}

void setup() {
    // Inicializar Serial
    Serial.begin(115200);
    while (!Serial) {
        ; // Esperar a que se inicie el puerto serial
    }
    Serial.println("Interfaz serial inicializada.");
    Serial.println("Ingrese comandos. Ejemplo:");
    Serial.println("SET PARAM=valor");
    Serial.println("START");
    Serial.println("STOP");
    Serial.println("STATUS");

    // Configurar GPIOs de dirección
    pinMode(GPIO_DISCO_DIR, OUTPUT);
    pinMode(GPIO_BOLA_DIR, OUTPUT);
    digitalWrite(GPIO_DISCO_DIR, HIGH); // Ajustar según la dirección deseada
    digitalWrite(GPIO_BOLA_DIR, HIGH);

    // Configurar LEDC
    configurarLEDC();

    // Configurar Celda de Carga
    configurarCeldaCarga();

    // Variables de control
    aceleracion_activa = false;
    tiempo_anterior = millis();
}

void loop() {
    // Procesar comandos seriales
    if (Serial.available() > 0) {
        char comando[64];
        int len = Serial.readBytesUntil('\n', comando, sizeof(comando) - 1);
        comando[len] = '\0'; // Añadir terminador nulo
        procesarComando(comando);
    }

    // Control de aceleración
    if (aceleracion_activa) {
        unsigned long tiempo_actual = millis();
        // Actualizar frecuencias cada dt segundos
        if ((tiempo_actual - tiempo_anterior) >= (dt * 1000)) {
            tiempo_anterior = tiempo_actual;
            actualizarFrecuencias();
            // Verificar si se alcanzó el tiempo total de aceleración
            if (t_global >= T) {
                aceleracion_activa = false;
                Serial.println("Aceleracion completada.");
                // Detener motores o mantener velocidad constante
                // ledc_stop(LEDC_MODE, LEDC_CHANNEL_DISCO, LOW);
                // ledc_stop(LEDC_MODE, LEDC_CHANNEL_BOLA, LOW);
            }
        }
    }
}

void procesarComando(char *comando) {
    if (strncmp(comando, "SET", 3) == 0) {
        // Procesar comandos SET
        // Ejemplo: SET R=5.4
        char *token = strtok(comando, " =");
        while (token != NULL) {
            if (strcmp(token, "R") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    R = atof(token);
                    Serial.print("R actualizado a: "); Serial.println(R);
                }
            } else if (strcmp(token, "r") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    r = atof(token);
                    Serial.print("r actualizado a: "); Serial.println(r);
                }
            } else if (strcmp(token, "RPM_D_i") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    RPM_D_i = atof(token);
                    Serial.print("RPM_D_i actualizado a: "); Serial.println(RPM_D_i);
                }
            } else if (strcmp(token, "RPM_D_f") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    RPM_D_f = atof(token);
                    Serial.print("RPM_D_f actualizado a: "); Serial.println(RPM_D_f);
                }
            } else if (strcmp(token, "mu") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    mu = atof(token);
                    Serial.print("mu actualizado a: "); Serial.println(mu);
                }
            } else if (strcmp(token, "T") == 0) {
                token = strtok(NULL, " =");
                if (token != NULL) {
                    T = atof(token);
                    Serial.print("T actualizado a: "); Serial.println(T);
                }
            }
            token = strtok(NULL, " =");
        }
    } else if (strncmp(comando, "START", 5) == 0) {
        if (!aceleracion_activa) {
            // Recalcular parámetros y activar aceleración
            omega_D_i = RPM_D_i * (2.0 * PI / 60.0);
            omega_D_f = RPM_D_f * (2.0 * PI / 60.0);
            a_D = (omega_D_f - omega_D_i) / T;
            K = (1 - mu) * (R / r);
            omega_B_i = K * omega_D_i;
            omega_B_f = K * omega_D_f;
            a_B = K * a_D;
            t_global = 0.0;
            tiempo_anterior = millis();
            tiempo_serial = millis();
            aceleracion_activa = true;
            mostrarCalculosIniciales();
            Serial.println("Iniciando aceleracion...");
        } else {
            Serial.println("La aceleracion ya está en curso.");
        }
    } else if (strncmp(comando, "STOP", 4) == 0) {
        // Detener motores
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_DISCO, LOW);
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_BOLA, LOW);
        aceleracion_activa = false;
        Serial.println("Motores detenidos.");
    } else if (strncmp(comando, "STATUS", 6) == 0) {
        // Mostrar estado actual
        Serial.println("=== Estado Actual ===");
        Serial.print("R: "); Serial.println(R);
        Serial.print("r: "); Serial.println(r);
        Serial.print("RPM_D_i: "); Serial.println(RPM_D_i);
        Serial.print("RPM_D_f: "); Serial.println(RPM_D_f);
        Serial.print("mu: "); Serial.println(mu);
        Serial.print("T: "); Serial.println(T);
        Serial.println("=====================");
    } else {
        Serial.print("Comando desconocido: ");
        Serial.println(comando);
    }
}
