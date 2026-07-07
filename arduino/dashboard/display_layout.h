#pragma once

#include "TFT_eSPI.h"
#include "dashboard_data.h"
#include "refresh_scheduler.h"

#ifdef EPAPER_ENABLE
class EPaper1Bit;
extern EPaper1Bit epaper;
#endif

namespace display {

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

Rect regionBounds(RefreshRegion region);

void begin();
void drawStaticChrome();
/** One full GC refresh after the initial draw. Partial updates stay disabled. */
void finishInitialDraw();
/** Redraw the entire sprite and run epaper.update() (full black/white flash). */
void fullScreenRefresh(const DashboardData& data, struct tm timeinfo);
void refreshRegion(RefreshRegion region, const DashboardData& data, struct tm timeinfo);

}  // namespace display
