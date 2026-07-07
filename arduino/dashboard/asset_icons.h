#pragma once

#include <stdint.h>

#ifdef EPAPER_ENABLE

namespace assets {

struct MonoBitmap {
  const uint8_t* data;
  uint16_t width;
  uint16_t height;
  uint16_t bytesPerRow;
};

MonoBitmap battery();
MonoBitmap humidity();
MonoBitmap rain();
MonoBitmap temperature();
MonoBitmap temperatureMin();
MonoBitmap temperatureMax();
MonoBitmap warning();
MonoBitmap stale();
MonoBitmap weatherIcon(const char* iconName);

}  // namespace assets

#endif
