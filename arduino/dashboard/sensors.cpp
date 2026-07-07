#include "sensors.h"

#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <cstddef>

namespace sensors {

static SensirionI2cSht4x sht4x;
static bool shtReady = false;

namespace {

struct BatteryCalPoint {
  float voltage;
  float percent;
};

constexpr BatteryCalPoint BATTERY_CAL[] = {
  {4.15f, 100.0f},
  {3.96f, 90.0f},
  {3.91f, 80.0f},
  {3.85f, 70.0f},
  {3.80f, 60.0f},
  {3.75f, 50.0f},
  {3.68f, 40.0f},
  {3.58f, 30.0f},
  {3.49f, 20.0f},
  {3.41f, 10.0f},
  {3.30f, 5.0f},
  {3.27f, 0.0f},
};
constexpr size_t BATTERY_CAL_COUNT = sizeof(BATTERY_CAL) / sizeof(BATTERY_CAL[0]);

}  // namespace

static uint8_t batteryPercentFromVoltage(float voltage) {
  if (voltage >= BATTERY_CAL[0].voltage) return 100;
  if (voltage <= BATTERY_CAL[BATTERY_CAL_COUNT - 1].voltage) return 0;

  for (size_t i = 0; i + 1 < BATTERY_CAL_COUNT; i++) {
    const BatteryCalPoint& hi = BATTERY_CAL[i];
    const BatteryCalPoint& lo = BATTERY_CAL[i + 1];
    if (voltage <= hi.voltage && voltage >= lo.voltage) {
      const float t = (voltage - hi.voltage) / (lo.voltage - hi.voltage);
      const float pct = hi.percent + t * (lo.percent - hi.percent);
      if (pct <= 0.0f) return 0;
      if (pct >= 100.0f) return 100;
      return static_cast<uint8_t>(pct + 0.5f);
    }
  }

  return 0;
}

static float readBatteryVoltage() {
  digitalWrite(BATTERY_ENABLE_PIN, HIGH);
  delay(10);
  const int mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  digitalWrite(BATTERY_ENABLE_PIN, LOW);
  return (mv / 1000.0f) * 2.0f;
}

void begin() {
  pinMode(BATTERY_ENABLE_PIN, OUTPUT);
  digitalWrite(BATTERY_ENABLE_PIN, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  Wire.begin(I2C_SDA, I2C_SCL);
  sht4x.begin(Wire, 0x44);

  float t = 0.0f;
  float h = 0.0f;
  shtReady = (sht4x.measureHighPrecision(t, h) == 0);
}

bool readEnvironment(float& tempC, float& humidityPct) {
  if (!shtReady) return false;

  float t = 0.0f;
  float h = 0.0f;
  if (sht4x.measureHighPrecision(t, h) != 0) return false;

  tempC = t;
  humidityPct = h;
  return true;
}

uint8_t readBatteryPercent() {
  return batteryPercentFromVoltage(readBatteryVoltage());
}

}  // namespace sensors
