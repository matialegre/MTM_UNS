#ifndef CONTROLADOR_MOTOR_H
#define CONTROLADOR_MOTOR_H

#include "driver/gpio.h"
#include "driver/mcpwm.h"

class MotorController {
public:
    MotorController(gpio_num_t pin_paso, gpio_num_t pin_direccion);
    void set_speed(int rpm);
    void stop();
    bool verify_operation();
private:
    gpio_num_t pin_paso;
    gpio_num_t pin_direccion;
    int current_rpm;
    float rpm_to_duty_cycle(int rpm);
};

#endif
