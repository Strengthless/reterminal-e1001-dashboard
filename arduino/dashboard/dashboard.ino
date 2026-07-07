#include "driver.h"
#include "config.h"
#include "dashboard_data.h"
#include "epaper_setup.h"
#include "display_layout.h"
#include "dashboard_network.h"
#include "refresh_scheduler.h"
#include "sensors.h"

#ifdef EPAPER_ENABLE
EPaper1Bit epaper;
#endif

#include <time.h>
#include <esp_sleep.h>

DashboardData dashboard;
RefreshScheduler scheduler;

static void syncTimeFromNtp() {
  // configTime() would overwrite TZ — use configTzTime for UK GMT/BST (Europe/London).
  configTzTime("GMT0BST,M3.5.0/1,M10.5.0", "pool.ntp.org", "time.nist.gov");
}

static bool waitForTimeSync(uint32_t timeoutMs = 10000) {
  tm timeinfo {};
  const uint32_t deadline = millis() + timeoutMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (getLocalTime(&timeinfo)) return true;
    delay(100);
  }
  return false;
}

static bool readTime(tm& timeinfo) {
  if (!getLocalTime(&timeinfo)) return false;
  return true;
}

static void updateSensors() {
  sensors::readEnvironment(dashboard.sensorTempC, dashboard.sensorHumidityPct);
  dashboard.batteryPct = sensors::readBatteryPercent();
}

void setup() {
  Serial.begin(115200);

  dashboardDataInit(dashboard);

  sensors::begin();
  network::begin();
  scheduler.begin();
  syncTimeFromNtp();

  updateSensors();
  network::fetchDashboard(dashboard);
  waitForTimeSync();
#if LAYOUT_PREVIEW
  applyLayoutPreviewMock(dashboard);
#endif

#ifdef EPAPER_ENABLE
  display::begin();
  tm timeinfo {};
  if (readTime(timeinfo)) {
    display::fullScreenRefresh(dashboard, timeinfo);
  }
#endif

  scheduler.markRan(RefreshRegion::Clock, millis());
  scheduler.markRan(RefreshRegion::Date, millis());
  scheduler.markRan(RefreshRegion::Battery, millis());
  scheduler.markRan(RefreshRegion::Sensors, millis());
  scheduler.markRan(RefreshRegion::Tfl, millis());
  scheduler.markRan(RefreshRegion::Weather, millis());
}

void loop() {
  const uint32_t now = millis();
  tm timeinfo {};
  bool needsFullRefresh = false;

  if (!readTime(timeinfo)) {
    delay(200);
    return;
  }

  if (scheduler.tasks[4].due(now)) {
    network::fetchTfl(dashboard);
    scheduler.markRan(RefreshRegion::Tfl, now);
    needsFullRefresh = true;
  }

  if (scheduler.tasks[2].due(now)) {
    updateSensors();
    scheduler.markRan(RefreshRegion::Battery, now);
    scheduler.markRan(RefreshRegion::Sensors, now);
    needsFullRefresh = true;
  }

  if (scheduler.tasks[5].due(now)) {
    network::fetchWeather(dashboard);
    scheduler.markRan(RefreshRegion::Weather, now);
    needsFullRefresh = true;
  }

  if (scheduler.dateDue(now, timeinfo.tm_yday)) {
    scheduler.markRan(RefreshRegion::Date, now);
    needsFullRefresh = true;
  }

  if (scheduler.clockDue(now)) {
    scheduler.markRan(RefreshRegion::Clock, now);
    needsFullRefresh = true;
  }

#ifdef EPAPER_ENABLE
  if (needsFullRefresh) {
#if LAYOUT_PREVIEW
    applyLayoutPreviewMock(dashboard);
#endif
    display::fullScreenRefresh(dashboard, timeinfo);
  }
#endif

  const uint32_t sleepMs = scheduler.msUntilNext(millis());
  if (sleepMs > 100) {
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepMs) * 1000ULL);
    esp_light_sleep_start();
  } else {
    delay(50);
  }
}
