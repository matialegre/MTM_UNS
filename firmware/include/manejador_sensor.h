#ifndef MANEJADOR_SENSOR_H
#define MANEJADOR_SENSOR_H

#include "driver/adc.h"

class TorqueSensor {
public:
    TorqueSensor(gpio_num_t pin_adc);
    float read_torque();
    void calibrate(float slope, float intercept);
private:
    float calculate_torque(float voltage);
    float slope, intercept;
};

#endif
