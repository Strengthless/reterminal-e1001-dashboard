#!/usr/bin/env node

import { readFileSync, writeFileSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const DASHBOARD = join(ROOT, 'arduino', 'dashboard');
const manifest = readFileSync(join(DASHBOARD, 'assets', 'manifest.txt'), 'utf8')
  .split('\n')
  .map((l) => l.trim().replace(/\\/g, '/'))
  .filter((l) => l.includes('/wi_') && l.endsWith('.h'));

const icons = manifest.map((p) => {
  const base = p.split('/').pop().replace('.h', '');
  const name = base.replace(/_/g, '-');
  return { base, name };
});

const includes = [
  '#include "assets/battery.h"',
  '#include "assets/humidity.h"',
  '#include "assets/rain.h"',
  '#include "assets/temperature.h"',
  '#include "assets/temperature_min.h"',
  '#include "assets/temperature_max.h"',
  '#include "assets/warning.h"',
  '#include "assets/stale.h"',
  ...icons.map((i) => `#include "assets/${i.base}.h"`),
].join('\n');

const cases = icons.map((i) => `  if (strcmp(iconName, "${i.name}") == 0) {
    return { asset_${i.base}_data, asset_${i.base}_width, asset_${i.base}_height, asset_${i.base}_bytes_per_row };
  }`).join('\n');

const cpp = `#include "epaper_setup.h"
#include "asset_icons.h"

#include <cstring>

${includes}

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

${cases}

  return make(asset_wi_na_data, asset_wi_na_width, asset_wi_na_height, asset_wi_na_bytes_per_row);
}

}  // namespace assets
`;

writeFileSync(join(DASHBOARD, 'asset_icons.cpp'), cpp);
console.log(`Wrote arduino/dashboard/asset_icons.cpp (${icons.length} weather icons)`);
