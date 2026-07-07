#include "dashboard_network.h"

#include "config.h"
#include "dashboard_diag.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>

namespace network {

namespace {

static bool wifiStarted = false;
static char lastTflFailReason[24] = {};
static uint8_t tflFailStreak = 0;

constexpr int kFetchMaxAttempts = 3;
constexpr uint32_t kFetchRetryDelayMs = 300;
constexpr uint8_t kTflStaleAfterFails = 2;
constexpr uint32_t kIpWaitMs = 2000;

const char* wifiStatusStr(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void setLastTflFailReason(const char* reason) {
  strncpy(lastTflFailReason, reason, sizeof(lastTflFailReason) - 1);
  lastTflFailReason[sizeof(lastTflFailReason) - 1] = '\0';
}

void clearLastTflFailReason() {
  lastTflFailReason[0] = '\0';
}

enum class FetchFail : uint8_t {
  None,
  Wifi,
  HttpBegin,
  HttpStatus,
  Json,
};

const char* fetchFailStr(FetchFail fail) {
  switch (fail) {
    case FetchFail::Wifi: return "wifi";
    case FetchFail::HttpBegin: return "http_begin";
    case FetchFail::HttpStatus: return "http_status";
    case FetchFail::Json: return "json";
    default: return "-";
  }
}

struct FetchResult {
  bool ok = false;
  FetchFail fail = FetchFail::None;
  int httpCode = 0;
  uint32_t elapsedMs = 0;
};

static bool wifiHasIp() {
  return WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

static bool waitForIp(uint32_t timeoutMs) {
  const uint32_t deadline = millis() + timeoutMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (WiFi.status() == WL_CONNECTED && wifiHasIp()) return true;
    delay(50);
  }
  return WiFi.status() == WL_CONNECTED && wifiHasIp();
}

static bool isTransientFetchFailure(const FetchResult& result) {
  if (result.fail == FetchFail::Wifi) return true;
  if (result.fail != FetchFail::HttpStatus) return false;
  return result.httpCode <= 0;
}

void logFetchLine(const char* path, const FetchResult& result) {
  if (result.ok) {
    diag::printf("[net] %s ok http=%d ms=%lu",
                 path,
                 result.httpCode,
                 static_cast<unsigned long>(result.elapsedMs));
  } else {
    diag::printf("[net] %s fail reason=%s http=%d ms=%lu",
                 path,
                 fetchFailStr(result.fail),
                 result.httpCode,
                 static_cast<unsigned long>(result.elapsedMs));
  }
}

}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  wifiStarted = true;
}

void afterWake() {
  delay(100);
  if (waitForIp(kIpWaitMs)) return;

  diag::printf("[net] after wake no ip wifi=%s", wifiStatusStr(WiFi.status()));
  WiFi.reconnect();
  if (waitForIp(3000)) return;
  ensureConnected(5000);
}

bool ensureConnected(uint32_t timeoutMs) {
  if (!wifiStarted) begin();

  if (WiFi.status() == WL_CONNECTED && wifiHasIp()) return true;
  if (WiFi.status() == WL_CONNECTED && waitForIp(kIpWaitMs)) return true;

  if (WIFI_SSID[0] == '\0') {
    diag::println("[net] wifi skip: SSID not configured");
    return false;
  }

  diag::printf("[net] wifi connect ssid=%s was=%s",
               WIFI_SSID,
               wifiStatusStr(WiFi.status()));

  WiFi.disconnect(false);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const uint32_t deadline = millis() + timeoutMs;
  while (WiFi.status() != WL_CONNECTED && (int32_t)(deadline - millis()) > 0) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    waitForIp(kIpWaitMs);
  }

  const bool connected = WiFi.status() == WL_CONNECTED && wifiHasIp();
  if (connected) {
    diag::printf("[net] wifi connected rssi=%d ip=%s",
                 WiFi.RSSI(),
                 WiFi.localIP().toString().c_str());
  } else {
    diag::printf("[net] wifi connect failed status=%s", wifiStatusStr(WiFi.status()));
    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      diag::printf("[net] SSID not found: \"%s\" — scanning...", WIFI_SSID);
      const int found = WiFi.scanNetworks();
      if (found <= 0) {
        diag::println("[net] scan found no networks");
      } else {
        const int limit = found > 25 ? 25 : found;
        for (int i = 0; i < limit; i++) {
          diag::printf("[net]   [%d] \"%s\" rssi=%d ch=%d",
                       i,
                       WiFi.SSID(i).c_str(),
                       WiFi.RSSI(i),
                       WiFi.channel(i));
        }
        if (found > limit) {
          diag::printf("[net]   ... %d more", found - limit);
        }
      }
      WiFi.scanDelete();
    }
  }
  return connected;
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

static FetchResult fetchJsonPathOnce(const char* path,
                                     JsonDocument& doc,
                                     uint32_t connectTimeoutMs,
                                     uint32_t httpTimeoutMs) {
  FetchResult result;

  if (!ensureConnected(connectTimeoutMs)) {
    result.fail = FetchFail::Wifi;
    return result;
  }

  HTTPClient http;
  http.setTimeout(httpTimeoutMs);
  http.setReuse(false);

  const String url = String(WORKER_URL) + path;
  if (!http.begin(url)) {
    result.fail = FetchFail::HttpBegin;
    diag::printf("[net] %s http.begin failed", path);
    return result;
  }

  const uint32_t t0 = millis();
  result.httpCode = http.GET();
  result.elapsedMs = millis() - t0;

  if (result.httpCode != HTTP_CODE_OK) {
    result.fail = FetchFail::HttpStatus;
    diag::printf("[net] %s http error=%s",
                  path,
                  http.errorToString(result.httpCode).c_str());
    http.end();
    return result;
  }

  const String payload = http.getString();
  http.end();

  const DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    result.fail = FetchFail::Json;
    diag::printf("[net] %s json error=%s", path, err.c_str());
    return result;
  }

  result.ok = true;
  return result;
}

static FetchResult fetchJsonPath(const char* path,
                                 JsonDocument& doc,
                                 uint32_t connectTimeoutMs,
                                 uint32_t httpTimeoutMs) {
  FetchResult last {};
  for (int attempt = 1; attempt <= kFetchMaxAttempts; attempt++) {
    last = fetchJsonPathOnce(path, doc, connectTimeoutMs, httpTimeoutMs);
    if (last.ok) return last;
    if (!isTransientFetchFailure(last) || attempt == kFetchMaxAttempts) return last;

    diag::printf("[net] %s retry %d/%d", path, attempt, kFetchMaxAttempts);
    delay(kFetchRetryDelayMs);

    if (WiFi.status() != WL_CONNECTED || !wifiHasIp()) {
      ensureConnected(connectTimeoutMs);
    }
  }
  return last;
}

bool fetchTfl(DashboardData& data, uint32_t connectTimeoutMs) {
  const bool wasStale = data.tflStale;

  JsonDocument doc;
  const FetchResult result =
      fetchJsonPath("tfl", doc, connectTimeoutMs, 8000);
  logFetchLine("tfl", result);

  if (!result.ok) {
    setLastTflFailReason(fetchFailStr(result.fail));
    tflFailStreak++;
    if (data.tflOk && tflFailStreak >= kTflStaleAfterFails) {
      data.tflStale = true;
      diag::printf("[net] tfl stale reason=%s", lastTflFailReason);
    }
    return false;
  }

  parseTfl(doc["tfl"].as<JsonObject>(), data);
  data.tflOk = true;
  tflFailStreak = 0;
  clearLastTflFailReason();
  if (wasStale) {
    diag::printf("[net] tfl fresh etas=%u", data.tflEtaCount);
  }
  data.tflStale = false;
  return true;
}

bool fetchWeather(DashboardData& data, uint32_t connectTimeoutMs) {
  JsonDocument doc;
  const FetchResult result =
      fetchJsonPath("weather", doc, connectTimeoutMs, 15000);
  if (!result.ok) {
    logFetchLine("weather", result);
    return false;
  }

  parseWeather(doc["weather"].as<JsonObject>(), data);
  data.weatherOk = true;
  return true;
}

bool fetchDashboard(DashboardData& data) {
  const bool tflOk = fetchTfl(data, 15000);
  const bool weatherOk = fetchWeather(data, 15000);
  return tflOk || weatherOk;
}

}  // namespace network
