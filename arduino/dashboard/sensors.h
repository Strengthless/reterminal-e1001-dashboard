#pragma once

#include <Arduino.h>

namespace sensors {

constexpr int BATTERY_ADC_PIN = 1;
constexpr int BATTERY_ENABLE_PIN = 21;
constexpr int I2C_SDA = 19;
constexpr int I2C_SCL = 20;

void begin();
bool readEnvironment(float& tempC, float& humidityPct);
uint8_t readBatteryPercent();

}  // namespace sensors
