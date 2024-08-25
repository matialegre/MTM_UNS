#include "manejador_sensor.h"

TorqueSensor::TorqueSensor(gpio_num_t pin_adc) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)pin_adc, ADC_ATTEN_DB_11);
}

float TorqueSensor::read_torque() {
    int raw = adc1_get_raw((adc1_channel_t)GPIO_NUM_34);
    float voltage = (float)raw / 4096.0 * 3.3;
    return calculate_torque(voltage);
}

void TorqueSensor::calibrate(float slope, float intercept) {
    this->slope = slope;
    this->intercept = intercept;
}

float TorqueSensor::calculate_torque(float voltage) {
    return slope * voltage + intercept;
}
