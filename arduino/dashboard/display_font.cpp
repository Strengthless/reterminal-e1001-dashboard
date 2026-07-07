#include "epaper_setup.h"
#include "display_font.h"

#ifdef USE_MONTSERRAT
#include "fonts/Montserrat_30.h"
#include "fonts/Montserrat_26.h"
#include "fonts/Montserrat_24.h"
#include "fonts/Montserrat_17.h"
#include "fonts/Montserrat_17_Bold.h"
#include "fonts/Montserrat_16.h"

extern EPaper1Bit epaper;

namespace display {

static FontRole activeFont = FontRole::Header;

void setFont(FontRole role) {
  epaper.unloadFont();

  switch (role) {
    case FontRole::Header: epaper.loadFont(Montserrat_30); break;
    case FontRole::Title: epaper.loadFont(Montserrat_24); break;
    case FontRole::Subtitle: epaper.loadFont(Montserrat_16); break;
    case FontRole::SensorValue: epaper.loadFont(Montserrat_26); break;
    case FontRole::WeatherHourly: epaper.loadFont(Montserrat_17); break;
    case FontRole::WeatherHourlyBold: epaper.loadFont(Montserrat_17_Bold); break;
  }

  activeFont = role;
}

void unsetFont() {
  epaper.unloadFont();
  activeFont = FontRole::Header;
}

}  // namespace display

#endif  // USE_MONTSERRAT
