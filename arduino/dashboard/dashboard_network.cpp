#include "dashboard_network.h"

#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>

namespace network {

static bool wifiStarted = false;

void begin() {
  WiFi.mode(WIFI_STA);
  wifiStarted = true;
}

bool ensureConnected() {
  if (!wifiStarted) begin();
  if (WiFi.status() == WL_CONNECTED) return true;
  if (WIFI_SSID[0] == '\0') return false;

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  const uint32_t deadline = millis() + 15000;
  while (WiFi.status() != WL_CONNECTED && (int32_t)(deadline - millis()) > 0) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

static void copyString(char* dest, size_t size, const char* src) {
  if (!src) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

static void parseTfl(JsonObject tfl, DashboardData& data) {
  copyString(data.tflLine, sizeof(data.tflLine), tfl["line"]);
  copyString(data.tflStop, sizeof(data.tflStop), tfl["stop"]);
  copyString(data.tflDirection, sizeof(data.tflDirection), tfl["direction"]);
  copyString(data.tflStatus, sizeof(data.tflStatus), tfl["status"]);
  data.tflDisruption = tfl["disruption"] | false;

  data.tflEtaCount = 0;
  JsonArray etas = tfl["etas"].as<JsonArray>();
  for (JsonObject eta : etas) {
    if (data.tflEtaCount >= 4) break;
    TflEta& row = data.tflEtas[data.tflEtaCount++];
    copyString(row.destination, sizeof(row.destination), eta["destination"]);
    row.minutes = eta["minutes"] | 0;
  }
}

static void parseWeather(JsonObject weather, DashboardData& data) {
  copyString(data.weatherLocation, sizeof(data.weatherLocation), weather["location"]);
  copyString(data.weatherDescription, sizeof(data.weatherDescription), weather["description"]);
  copyString(data.weatherIcon, sizeof(data.weatherIcon), weather["icon"]);
  data.weatherTempC = weather["tempC"].isNull() ? NAN : weather["tempC"].as<float>();
  data.weatherFeelsLikeC = weather["feelsLikeC"].isNull() ? NAN : weather["feelsLikeC"].as<float>();
  data.weatherMinC = weather["minC"].isNull() ? NAN : weather["minC"].as<float>();
  data.weatherMaxC = weather["maxC"].isNull() ? NAN : weather["maxC"].as<float>();
  data.weatherPrecipMm = weather["precipMm"].isNull() ? NAN : weather["precipMm"].as<float>();
}

static bool fetchJsonPath(const char* path, JsonDocument& doc) {
  if (!ensureConnected()) return false;

  HTTPClient http;
  http.setTimeout(10000);

  String url = String(WORKER_URL) + path;
  if (!http.begin(url)) return false;

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  return !deserializeJson(doc, payload);
}

bool fetchTfl(DashboardData& data) {
  JsonDocument doc;
  if (!fetchJsonPath("tfl", doc)) {
    if (data.apiOk) data.apiStale = true;
    return false;
  }

  parseTfl(doc["tfl"].as<JsonObject>(), data);
  data.apiOk = true;
  data.apiStale = false;
  return true;
}

bool fetchWeather(DashboardData& data) {
  JsonDocument doc;
  if (!fetchJsonPath("weather", doc)) {
    if (data.apiOk) data.apiStale = true;
    return false;
  }

  parseWeather(doc["weather"].as<JsonObject>(), data);
  data.apiOk = true;
  data.apiStale = false;
  return true;
}

bool fetchDashboard(DashboardData& data) {
  const bool tflOk = fetchTfl(data);
  const bool weatherOk = fetchWeather(data);
  return tflOk || weatherOk;
}

}  // namespace network
