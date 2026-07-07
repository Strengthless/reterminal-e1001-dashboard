#pragma once

#include "dashboard_data.h"

namespace network {

void begin();
void afterWake();
bool ensureConnected(uint32_t timeoutMs = 15000);
bool fetchTfl(DashboardData& data, uint32_t connectTimeoutMs = 4000);
bool fetchWeather(DashboardData& data, uint32_t connectTimeoutMs = 8000);
bool fetchDashboard(DashboardData& data);

}  // namespace network
