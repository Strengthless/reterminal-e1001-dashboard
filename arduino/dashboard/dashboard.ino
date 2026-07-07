#include "driver.h"
#include "config.h"
#include "dashboard_data.h"
#include "epaper_setup.h"
#include "display_layout.h"
#include "dashboard_network.h"
#include "refresh_scheduler.h"
#include "sensors.h"
#include "dashboard_diag.h"

#ifdef EPAPER_ENABLE
EPaper1Bit epaper;
#endif

#include <time.h>
#include <esp_sleep.h>

DashboardData dashboard;
RefreshScheduler scheduler;

// Light sleep drops the TCP stack while WiFi.status() stays CONNECTED.
// Keep WiFi alive for short intervals so /tfl fetches stay fast.
constexpr uint32_t kLightSleepThresholdMs = 60 * 1000;

static void syncTimeFromNtp() {
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
  return getLocalTime(&timeinfo);
}

static void updateSensors() {
  sensors::readEnvironment(dashboard.sensorTempC, dashboard.sensorHumidityPct);
  dashboard.batteryPct = sensors::readBatteryPercent();
}

static bool hasApiData() {
  return dashboard.tflOk || dashboard.weatherOk;
}

static void refreshDisplay(const tm& timeinfo) {
#ifdef EPAPER_ENABLE
  display::fullScreenRefresh(dashboard, timeinfo);
#endif
}

static void maybeRetryTimeSync(uint32_t now) {
  static uint32_t lastRetryMs = 0;
  if (now - lastRetryMs < 60000) return;
  lastRetryMs = now;

  tm probe {};
  if (readTime(probe)) return;
  if (!network::ensureConnected()) return;

  diag::println("[time] retry NTP sync");
  syncTimeFromNtp();
  waitForTimeSync(5000);
}

void setup() {
  diag::begin();

  dashboardDataInit(dashboard);

  sensors::begin();
  network::begin();
  scheduler.begin();

  updateSensors();

  const bool wifiUp = network::ensureConnected();
  if (wifiUp) {
    syncTimeFromNtp();
  } else {
    diag::println("[setup] wifi unavailable — skipping NTP");
  }

  network::fetchDashboard(dashboard);

  if (wifiUp) {
    waitForTimeSync();
  }

#if LAYOUT_PREVIEW
  applyLayoutPreviewMock(dashboard);
#endif

#ifdef EPAPER_ENABLE
  display::begin();

  if (hasApiData()) {
    tm timeinfo {};
    readTime(timeinfo);
    refreshDisplay(timeinfo);
  }
#endif

  const uint32_t done = millis();
  scheduler.markRan(RefreshRegion::Clock, done);
  scheduler.markRan(RefreshRegion::Date, done);
  scheduler.markRan(RefreshRegion::Battery, done);
  scheduler.markRan(RefreshRegion::Sensors, done);
  scheduler.markRan(RefreshRegion::Tfl, done);
  scheduler.markRan(RefreshRegion::Weather, done);
}

void loop() {
  const uint32_t loopStart = millis();
  bool needsFullRefresh = false;
  bool ranTfl = false;
  bool ranClock = false;
  bool ranDate = false;
  bool ranBattery = false;
  bool ranSensors = false;
  bool ranWeather = false;

  tm timeinfo {};
  const bool hasTime = readTime(timeinfo);
  if (!hasTime) {
    maybeRetryTimeSync(loopStart);
  }

  if (scheduler.periodicDue(loopStart)) {
    network::fetchTfl(dashboard);
    ranTfl = true;
    ranClock = true;
    needsFullRefresh = true;
  }

  if (scheduler.sensorsDue(loopStart)) {
    updateSensors();
    ranBattery = true;
    ranSensors = true;
    needsFullRefresh = true;
  }

  if (scheduler.weatherDue(loopStart)) {
    network::fetchWeather(dashboard);
    ranWeather = true;
    needsFullRefresh = true;
  }

  if (hasTime && scheduler.dateDue(loopStart, timeinfo.tm_yday)) {
    ranDate = true;
    needsFullRefresh = true;
  }

  uint32_t markMs = millis();
  if (needsFullRefresh && hasApiData()) {
#if LAYOUT_PREVIEW
    applyLayoutPreviewMock(dashboard);
#endif
    readTime(timeinfo);
    refreshDisplay(timeinfo);
    markMs = millis();
  }

  if (ranTfl) scheduler.markRan(RefreshRegion::Tfl, markMs);
  if (ranBattery) scheduler.markRan(RefreshRegion::Battery, markMs);
  if (ranSensors) scheduler.markRan(RefreshRegion::Sensors, markMs);
  if (ranWeather) scheduler.markRan(RefreshRegion::Weather, markMs);
  if (ranDate) scheduler.markRan(RefreshRegion::Date, markMs);
  if (ranClock) scheduler.markRan(RefreshRegion::Clock, markMs);

  const uint32_t sleepMs = scheduler.msUntilNext(millis());
  if (sleepMs > 100) {
    if (sleepMs <= kLightSleepThresholdMs) {
      delay(sleepMs);
    } else {
      esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepMs) * 1000ULL);
      esp_light_sleep_start();
      network::afterWake();
    }
  } else {
    delay(50);
  }
}
