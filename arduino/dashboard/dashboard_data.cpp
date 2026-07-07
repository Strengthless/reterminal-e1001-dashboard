#include "dashboard_data.h"
#include <cstring>

void dashboardDataInit(DashboardData& data) {
  std::memset(&data, 0, sizeof(data));
  std::strncpy(data.tflStatus, "N/A", sizeof(data.tflStatus));
  std::strncpy(data.weatherDescription, "N/A", sizeof(data.weatherDescription));
  std::strncpy(data.weatherIcon, "wi-na", sizeof(data.weatherIcon));
}

#if LAYOUT_PREVIEW
void applyLayoutPreviewMock(DashboardData& data) {
  std::strncpy(data.tflStatus, "Good Service", sizeof(data.tflStatus));
  data.tflDisruption = false;
  data.apiOk = true;
  data.apiStale = true;

  std::strncpy(data.weatherLocation, "Ealing, London", sizeof(data.weatherLocation));
  std::strncpy(data.weatherDescription, "Overcast", sizeof(data.weatherDescription));
  std::strncpy(data.weatherIcon, "wi-cloudy", sizeof(data.weatherIcon));
  data.weatherTempC = 28.9f;
  data.weatherFeelsLikeC = 27.5f;
  data.weatherMinC = 17.2f;
  data.weatherMaxC = 29.0f;
  data.weatherPrecipMm = 0.0f;
}
#endif
