#pragma once

#include "TFT_eSPI.h"

#ifdef EPAPER_ENABLE

#if defined(__has_include)
#  if __has_include("fonts/Montserrat_24.h")
#    define USE_MONTSERRAT
#  endif
#endif

// Or uncomment manually after generating VLW headers — see arduino/fonts/README.md
// #define USE_MONTSERRAT

namespace display {

enum class FontRole {
  Header,            // 30px — top bar
  Title,             // 24px — section titles, ETAs, daily weather
  Subtitle,          // 16px — TfL subtitle, status
  SensorValue,       // 26px — bedroom temp / humidity
  WeatherHourly,     // 17px — condition + temp line
  WeatherHourlyBold, // 17px bold — condition label
};

#ifdef USE_MONTSERRAT
void setFont(FontRole role);
void unsetFont();
#else
inline void setFont(FontRole) {}
inline void unsetFont() {}
#endif

}  // namespace display

#endif
