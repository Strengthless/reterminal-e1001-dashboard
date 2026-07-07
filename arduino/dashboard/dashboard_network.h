#pragma once

#include "dashboard_data.h"

namespace network {

void begin();
bool ensureConnected();
bool fetchDashboard(DashboardData& data);

}  // namespace network
