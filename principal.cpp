#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "controlador_motor.h"
#include "manejador_sensor.h"
#include "gestor_wifi.h"

#define UART_PORT UART_NUM_1
#define BUF_SIZE 1024

void inicializar_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

extern "C" void app_main(void) {
    inicializar_uart();
    iniciar_wifi();

    MotorController motor1(GPIO_NUM_18, GPIO_NUM_19);
    MotorController motor2(GPIO_NUM_21, GPIO_NUM_22);
    TorqueSensor sensor(GPIO_NUM_34);
    
    sensor.calibrate(2.5, 0);  // Ejemplo de calibración, ajustar según necesidades

    char data[BUF_SIZE];
    while (true) {
        int length = uart_read_bytes(UART_PORT, (uint8_t*)data, BUF_SIZE, pdMS_TO_TICKS(1000));
        if (length > 0) {
            data[length] = '\0';
            int rpm1 = atoi(data);
            motor1.set_speed(rpm1);
        }

        float torque = sensor.read_torque();
        snprintf(data, BUF_SIZE, "TORQUE:%.2f\n", torque);
        uart_write_bytes(UART_PORT, data, strlen(data));
        
        motor2.set_speed(600);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
