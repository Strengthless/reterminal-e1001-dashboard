#pragma once

#include <stdint.h>

// TEMP — set to 1 to force warning/stale icon and sample weather for layout check.
#ifndef LAYOUT_PREVIEW
#define LAYOUT_PREVIEW 0
#endif

struct TflEta {
  char destination[32];
  int minutes;
};

struct DashboardData {
  // TfL
  char tflLine[32];
  char tflStop[32];
  char tflDirection[16];
  char tflStatus[64];
  bool tflDisruption;
  TflEta tflEtas[4];
  uint8_t tflEtaCount;

  // Weather
  char weatherLocation[32];
  char weatherDescription[32];
  char weatherIcon[32];
  float weatherTempC;
  float weatherFeelsLikeC;
  float weatherMinC;
  float weatherMaxC;
  float weatherPrecipMm;

  // On-device sensors
  float sensorTempC;
  float sensorHumidityPct;
  uint8_t batteryPct;

  bool apiOk;
  bool apiStale;
};

void dashboardDataInit(DashboardData& data);

#if LAYOUT_PREVIEW
/** TEMP — overlay sample icons and weather for layout sanity check. */
void applyLayoutPreviewMock(DashboardData& data);
#endif
