#include "epaper_setup.h"
#include "asset_icons.h"

#include <cstring>

#include "assets/battery.h"
#include "assets/humidity.h"
#include "assets/rain.h"
#include "assets/temperature.h"
#include "assets/temperature_min.h"
#include "assets/temperature_max.h"
#include "assets/warning.h"
#include "assets/stale.h"
#include "assets/wi_cloudy.h"
#include "assets/wi_day_cloudy.h"
#include "assets/wi_day_hail.h"
#include "assets/wi_day_rain.h"
#include "assets/wi_day_showers.h"
#include "assets/wi_day_sleet.h"
#include "assets/wi_day_snow.h"
#include "assets/wi_day_storm_showers.h"
#include "assets/wi_day_sunny.h"
#include "assets/wi_fog.h"
#include "assets/wi_hail.h"
#include "assets/wi_na.h"
#include "assets/wi_night_alt_cloudy.h"
#include "assets/wi_night_alt_hail.h"
#include "assets/wi_night_alt_rain.h"
#include "assets/wi_night_alt_showers.h"
#include "assets/wi_night_alt_sleet.h"
#include "assets/wi_night_alt_snow.h"
#include "assets/wi_night_alt_storm_showers.h"
#include "assets/wi_night_clear.h"
#include "assets/wi_rain.h"
#include "assets/wi_sleet.h"
#include "assets/wi_snow.h"
#include "assets/wi_sprinkle.h"
#include "assets/wi_thunderstorm.h"

namespace assets {

namespace {

MonoBitmap make(const uint8_t* data, uint16_t width, uint16_t height, uint16_t bytesPerRow) {
  return { data, width, height, bytesPerRow };
}

}  // namespace

MonoBitmap battery() {
  return make(asset_battery_data, asset_battery_width, asset_battery_height, asset_battery_bytes_per_row);
}

MonoBitmap humidity() {
  return make(asset_humidity_data, asset_humidity_width, asset_humidity_height, asset_humidity_bytes_per_row);
}

MonoBitmap rain() {
  return make(asset_rain_data, asset_rain_width, asset_rain_height, asset_rain_bytes_per_row);
}

MonoBitmap temperature() {
  return make(asset_temperature_data, asset_temperature_width, asset_temperature_height, asset_temperature_bytes_per_row);
}

MonoBitmap temperatureMin() {
  return make(asset_temperature_min_data, asset_temperature_min_width, asset_temperature_min_height, asset_temperature_min_bytes_per_row);
}

MonoBitmap temperatureMax() {
  return make(asset_temperature_max_data, asset_temperature_max_width, asset_temperature_max_height, asset_temperature_max_bytes_per_row);
}

MonoBitmap warning() {
  return make(asset_warning_data, asset_warning_width, asset_warning_height, asset_warning_bytes_per_row);
}

MonoBitmap stale() {
  return make(asset_stale_data, asset_stale_width, asset_stale_height, asset_stale_bytes_per_row);
}

MonoBitmap weatherIcon(const char* iconName) {
  if (!iconName || !iconName[0]) {
    return make(asset_wi_na_data, asset_wi_na_width, asset_wi_na_height, asset_wi_na_bytes_per_row);
  }

  if (strcmp(iconName, "wi-cloudy") == 0) {
    return { asset_wi_cloudy_data, asset_wi_cloudy_width, asset_wi_cloudy_height, asset_wi_cloudy_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-cloudy") == 0) {
    return { asset_wi_day_cloudy_data, asset_wi_day_cloudy_width, asset_wi_day_cloudy_height, asset_wi_day_cloudy_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-hail") == 0) {
    return { asset_wi_day_hail_data, asset_wi_day_hail_width, asset_wi_day_hail_height, asset_wi_day_hail_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-rain") == 0) {
    return { asset_wi_day_rain_data, asset_wi_day_rain_width, asset_wi_day_rain_height, asset_wi_day_rain_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-showers") == 0) {
    return { asset_wi_day_showers_data, asset_wi_day_showers_width, asset_wi_day_showers_height, asset_wi_day_showers_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-sleet") == 0) {
    return { asset_wi_day_sleet_data, asset_wi_day_sleet_width, asset_wi_day_sleet_height, asset_wi_day_sleet_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-snow") == 0) {
    return { asset_wi_day_snow_data, asset_wi_day_snow_width, asset_wi_day_snow_height, asset_wi_day_snow_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-storm-showers") == 0) {
    return { asset_wi_day_storm_showers_data, asset_wi_day_storm_showers_width, asset_wi_day_storm_showers_height, asset_wi_day_storm_showers_bytes_per_row };
  }
  if (strcmp(iconName, "wi-day-sunny") == 0) {
    return { asset_wi_day_sunny_data, asset_wi_day_sunny_width, asset_wi_day_sunny_height, asset_wi_day_sunny_bytes_per_row };
  }
  if (strcmp(iconName, "wi-fog") == 0) {
    return { asset_wi_fog_data, asset_wi_fog_width, asset_wi_fog_height, asset_wi_fog_bytes_per_row };
  }
  if (strcmp(iconName, "wi-hail") == 0) {
    return { asset_wi_hail_data, asset_wi_hail_width, asset_wi_hail_height, asset_wi_hail_bytes_per_row };
  }
  if (strcmp(iconName, "wi-na") == 0) {
    return { asset_wi_na_data, asset_wi_na_width, asset_wi_na_height, asset_wi_na_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-cloudy") == 0) {
    return { asset_wi_night_alt_cloudy_data, asset_wi_night_alt_cloudy_width, asset_wi_night_alt_cloudy_height, asset_wi_night_alt_cloudy_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-hail") == 0) {
    return { asset_wi_night_alt_hail_data, asset_wi_night_alt_hail_width, asset_wi_night_alt_hail_height, asset_wi_night_alt_hail_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-rain") == 0) {
    return { asset_wi_night_alt_rain_data, asset_wi_night_alt_rain_width, asset_wi_night_alt_rain_height, asset_wi_night_alt_rain_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-showers") == 0) {
    return { asset_wi_night_alt_showers_data, asset_wi_night_alt_showers_width, asset_wi_night_alt_showers_height, asset_wi_night_alt_showers_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-sleet") == 0) {
    return { asset_wi_night_alt_sleet_data, asset_wi_night_alt_sleet_width, asset_wi_night_alt_sleet_height, asset_wi_night_alt_sleet_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-snow") == 0) {
    return { asset_wi_night_alt_snow_data, asset_wi_night_alt_snow_width, asset_wi_night_alt_snow_height, asset_wi_night_alt_snow_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-alt-storm-showers") == 0) {
    return { asset_wi_night_alt_storm_showers_data, asset_wi_night_alt_storm_showers_width, asset_wi_night_alt_storm_showers_height, asset_wi_night_alt_storm_showers_bytes_per_row };
  }
  if (strcmp(iconName, "wi-night-clear") == 0) {
    return { asset_wi_night_clear_data, asset_wi_night_clear_width, asset_wi_night_clear_height, asset_wi_night_clear_bytes_per_row };
  }
  if (strcmp(iconName, "wi-rain") == 0) {
    return { asset_wi_rain_data, asset_wi_rain_width, asset_wi_rain_height, asset_wi_rain_bytes_per_row };
  }
  if (strcmp(iconName, "wi-sleet") == 0) {
    return { asset_wi_sleet_data, asset_wi_sleet_width, asset_wi_sleet_height, asset_wi_sleet_bytes_per_row };
  }
  if (strcmp(iconName, "wi-snow") == 0) {
    return { asset_wi_snow_data, asset_wi_snow_width, asset_wi_snow_height, asset_wi_snow_bytes_per_row };
  }
  if (strcmp(iconName, "wi-sprinkle") == 0) {
    return { asset_wi_sprinkle_data, asset_wi_sprinkle_width, asset_wi_sprinkle_height, asset_wi_sprinkle_bytes_per_row };
  }
  if (strcmp(iconName, "wi-thunderstorm") == 0) {
    return { asset_wi_thunderstorm_data, asset_wi_thunderstorm_width, asset_wi_thunderstorm_height, asset_wi_thunderstorm_bytes_per_row };
  }

  return make(asset_wi_na_data, asset_wi_na_width, asset_wi_na_height, asset_wi_na_bytes_per_row);
}

}  // namespace assets
