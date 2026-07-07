#pragma once

#include "dashboard_data.h"

namespace network {

void begin();
bool ensureConnected();
bool fetchTfl(DashboardData& data);
bool fetchWeather(DashboardData& data);
bool fetchDashboard(DashboardData& data);

}  // namespace network
