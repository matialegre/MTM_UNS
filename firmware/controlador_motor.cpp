#include "controlador_motor.h"
#include "esp_log.h"

MotorController::MotorController(gpio_num_t pin_paso, gpio_num_t pin_direccion) {
    this->pin_paso = pin_paso;
    this->pin_direccion = pin_direccion;
    
    gpio_set_direction(pin_direccion, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin_paso, GPIO_MODE_OUTPUT);
    
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, pin_paso);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;
    pwm_config.cmpr_a = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}

void MotorController::set_speed(int rpm) {
    if (rpm < 0 || rpm > 2000) {
        ESP_LOGW("MotorController", "RPM fuera de rango: %d", rpm);
        return;
    }
    float duty_cycle = rpm_to_duty_cycle(rpm);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    current_rpm = rpm;
}

void MotorController::stop() {
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
}

bool MotorController::verify_operation() {
    // Implementar lógica de verificación de operación.
    return true;  // Placeholder
}

float MotorController::rpm_to_duty_cycle(int rpm) {
    return (float)rpm / 2000.0 * 100.0;
}
