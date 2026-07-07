#include "epaper_setup.h"
#include "display_layout.h"

#include "asset_icons.h"
#include "display_font.h"
#include "assets/elizabeth_line_roundel.h"

#include <time.h>
#include <stdio.h>
#include <cstring>
#include <math.h>

#ifdef EPAPER_ENABLE

namespace display {

bool partialEnabled = false;

namespace {

constexpr int SCREEN_W = 800;
constexpr int SCREEN_H = 480;
constexpr int BOARD_PAD = 20;
constexpr int SECTION_TOP_PAD = 28;
constexpr int SENSOR_H = 140;
constexpr int ETA_TOP_PAD = 26;
constexpr int ETA_FIRST_TOP_PAD = 12;
constexpr int ETA_BOTTOM_PAD = 18;
/** Line box for 24px title/ETA type — keep in sync with preview TYPO.title */
constexpr int ETA_LINE_H = 24;
constexpr int ETA_FIRST_ROW_H = ETA_FIRST_TOP_PAD + ETA_LINE_H + ETA_BOTTOM_PAD + 1;
constexpr int ETA_ROW_H = ETA_TOP_PAD + ETA_LINE_H + ETA_BOTTOM_PAD + 1;
constexpr int TFL_PAD_LEFT = 12;
constexpr int TFL_PAD_RIGHT = 28;
constexpr int TFL_ROUNDEL_GAP = 12;
constexpr int PANEL_PAD_LEFT = 20;
constexpr int PANEL_PAD = 16;
constexpr int HEADER_H = 56;
constexpr int HEADER_Y = BOARD_PAD;
constexpr int HEADER_LEFT_PAD = 8;
constexpr int BODY_Y = BOARD_PAD + HEADER_H;
constexpr int CONTENT_X = BOARD_PAD + TFL_PAD_LEFT;
constexpr int SPLIT_X = 464; // 58% of 800px
constexpr int ROUNDEL_X = CONTENT_X;
constexpr int ROUNDEL_Y = BODY_Y + SECTION_TOP_PAD;
constexpr int ROUNDEL_ETA_GAP = 16;  // space below roundel before ETA list
constexpr int ETA_TOP = ROUNDEL_Y + 64 + ROUNDEL_ETA_GAP;
constexpr int TFL_LIST_TOP = ETA_TOP;
constexpr int ETA_COUNT = 4;
constexpr int DOTTED_LINE_X1 = SPLIT_X - TFL_PAD_RIGHT;
constexpr int ETA_MINS_X = SPLIT_X - TFL_PAD_RIGHT - 96;
constexpr int ETA_MINS_W = 96;
/** Fixed pixel slot for ETA destination text (must cover longest station name + antialiasing). */
constexpr int ETA_DEST_SLOT_W = ETA_MINS_X - CONTENT_X - 8;
constexpr int EPD_PARTIAL_ALIGN = 8;
constexpr int HEADER_PIPE_GAP = 10;  // match preview .header-sep margin

int etaDottedY(uint8_t index) {
  if (index == 0) return TFL_LIST_TOP + ETA_FIRST_TOP_PAD + ETA_LINE_H + ETA_BOTTOM_PAD;
  return TFL_LIST_TOP + ETA_FIRST_ROW_H + (index - 1) * ETA_ROW_H + ETA_TOP_PAD + ETA_LINE_H + ETA_BOTTOM_PAD;
}

int etaRowTextY(uint8_t index) {
  if (index == 0) return TFL_LIST_TOP + ETA_FIRST_TOP_PAD;
  return TFL_LIST_TOP + ETA_FIRST_ROW_H + (index - 1) * ETA_ROW_H + ETA_TOP_PAD;
}

/**
 * CSS `justify-content: space-evenly` for a vertical stack.
 * n items → (n + 1) equal gaps (before first, between each pair, after last).
 *   gap = (containerH - sum(itemHeights)) / (itemCount + 1)
 *   y(i) = containerTop + (i + 1) * gap + sum(heights[0 .. i-1])
 * Preview: .panel-weather { justify-content: space-evenly; padding: 16px; }
 */
int spaceEvenlyY(int containerTop, int containerH, const int* itemHeights, uint8_t itemCount, uint8_t itemIndex) {
  int sumH = 0;
  for (uint8_t i = 0; i < itemCount; i++) sumH += itemHeights[i];

  const int gap = (containerH - sumH) / static_cast<int>(itemCount + 1);
  int y = containerTop + gap;
  for (uint8_t i = 0; i < itemIndex; i++) {
    y += itemHeights[i] + gap;
  }
  return y;
}

// Weather block heights — keep in sync with preview .panel-weather children
constexpr int WEATHER_ICON = 64;
constexpr int WEATHER_ICON_GAP = 6;
constexpr int WEATHER_HOURLY_LINE_H = 17;
constexpr int WEATHER_HOURLY_TEXT_GAP = 6;
constexpr int WEATHER_HOURLY_TEXT_BLOCK_H = WEATHER_HOURLY_LINE_H + WEATHER_HOURLY_TEXT_GAP + WEATHER_HOURLY_LINE_H;
// UI icon sizes — keep in sync with preview --icon-ui-size / --icon-battery-size
constexpr int UI_ICON_SIZE = 36;
constexpr int UI_ICON_TEXT_GAP = 4;
constexpr int STAT_TIGHT_GAP = 0;
constexpr int STAT_GROUP_GAP = 12;
constexpr int TFL_INLINE_GAP = 4;
constexpr int TFL_STALE_X_OFFSET = 2;  // extra nudge for hourglass only
constexpr int TFL_TITLE_ICON_Y_OFFSET = -3;  // nudge inline status icons up vs title cap height
constexpr int BATTERY_ICON_SIZE = 30;
constexpr int SENSOR_ROW_Y_OFFSET = 52;  // 28px label + 24px gap below title
constexpr int SENSOR_ROW_H = 36;
constexpr int SENSOR_ITEM_GAP = 32;
constexpr int SENSOR_VALUE_W = 76;
constexpr int WEATHER_LABEL_H = 28;  // .panel-label line box (24px title)
constexpr int WEATHER_MAIN_H = 64;   // .weather-main (64px icon; text fits within)
constexpr int WEATHER_STATS_H = UI_ICON_SIZE;  // .weather-stats row
constexpr uint8_t FS_HEADER = 4;   // 32px ≈ 30px spec
constexpr uint8_t FS_TITLE = 3;    // 24px
constexpr uint8_t FS_SUBTITLE = 2; // 16px
constexpr uint8_t FS_SENSOR = 3;   // 24px ≈ 26px spec
constexpr uint8_t FS_WEATHER_HOURLY = 2; // 16px ≈ 17px spec

uint8_t textSizeFor(FontRole role) {
  switch (role) {
    case FontRole::Header: return FS_HEADER;
    case FontRole::Title: return FS_TITLE;
    case FontRole::Subtitle: return FS_SUBTITLE;
    case FontRole::SensorValue: return FS_SENSOR;
    case FontRole::WeatherHourly:
    case FontRole::WeatherHourlyBold: return FS_WEATHER_HOURLY;
  }
  return FS_TITLE;
}

Rect headerDate() { return {BOARD_PAD, HEADER_Y + 8, 180, 40}; }
Rect headerClock() { return {254, HEADER_Y + 4, 220, 48}; }
Rect headerWeekday() { return {504, HEADER_Y + 8, 140, 40}; }
Rect headerBattery() { return {656, HEADER_Y + 4, 124, 48}; }
Rect tflPanel() { return {0, BODY_Y, SPLIT_X, SCREEN_H - BODY_Y - BOARD_PAD}; }
Rect sensorPanel() { return {SPLIT_X, BODY_Y, SCREEN_W - SPLIT_X, SENSOR_H}; }
Rect weatherPanel() { return {SPLIT_X, BODY_Y + SENSOR_H, SCREEN_W - SPLIT_X, SCREEN_H - BODY_Y - SENSOR_H - BOARD_PAD}; }

void drawMonoBitmap(int x, int y, const uint8_t* data, uint16_t width, uint16_t height, uint16_t bytesPerRow, uint16_t color) {
  for (uint16_t row = 0; row < height; row++) {
    for (uint16_t col = 0; col < width; col++) {
      const uint8_t byte = data[row * bytesPerRow + (col >> 3)];
      if (byte & (0x80 >> (col & 7))) {
        epaper.drawPixel(x + col, y + row, color);
      }
    }
  }
}

void drawDottedLine(int x0, int y0, int x1, int y1) {
  constexpr int DOT_SIZE = 2;
  constexpr int DOT_GAP = 8;
  for (int x = x0; x < x1; x += DOT_GAP) {
    epaper.fillRect(x, y0, DOT_SIZE, DOT_SIZE, TFT_BLACK);
  }
}

void redrawTflDottedLines() {
  for (int i = 0; i < ETA_COUNT; i++) {
    drawDottedLine(CONTENT_X, etaDottedY(i), DOTTED_LINE_X1, etaDottedY(i));
  }
}

void drawAsset(int x, int y, const assets::MonoBitmap& asset, uint16_t color = TFT_BLACK) {
  drawMonoBitmap(x, y, asset.data, asset.width, asset.height, asset.bytesPerRow, color);
}

void drawAssetVCentered(int x, int rowY, int rowH, const assets::MonoBitmap& asset) {
  const int iconY = rowY + (rowH - asset.height) / 2;
  drawAsset(x, iconY, asset);
}

void drawHeaderDivider() {
  epaper.drawFastHLine(BOARD_PAD, BODY_Y - 1, SCREEN_W - 2 * BOARD_PAD, TFT_BLACK);
  epaper.drawFastVLine(SPLIT_X - 1, BODY_Y, SCREEN_H - BODY_Y - BOARD_PAD, TFT_BLACK);
  epaper.drawFastHLine(SPLIT_X, BODY_Y + SENSOR_H, SCREEN_W - SPLIT_X - BOARD_PAD, TFT_BLACK);
}

void redrawHeaderRule() {
  epaper.drawFastHLine(BOARD_PAD, BODY_Y - 1, SCREEN_W - 2 * BOARD_PAD, TFT_BLACK);
}

void redrawBodyDividers() {
  epaper.drawFastVLine(SPLIT_X - 1, BODY_Y, SCREEN_H - BODY_Y - BOARD_PAD, TFT_BLACK);
  epaper.drawFastHLine(SPLIT_X, BODY_Y + SENSOR_H, SCREEN_W - SPLIT_X - BOARD_PAD, TFT_BLACK);
}

void drawRoundel() {
  drawMonoBitmap(
    ROUNDEL_X, ROUNDEL_Y,
    asset_elizabeth_line_roundel_data,
    asset_elizabeth_line_roundel_width,
    asset_elizabeth_line_roundel_height,
    asset_elizabeth_line_roundel_bytes_per_row,
    TFT_BLACK);
}

void formatClock(char* buf, size_t len, const tm& timeinfo) {
  snprintf(buf, len, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void formatDate(char* buf, size_t len, const tm& timeinfo) {
  snprintf(buf, len, "%04d.%02d.%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
}

void formatWeekday(char* buf, size_t len, const tm& timeinfo) {
  static const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  snprintf(buf, len, "%s", days[timeinfo.tm_wday]);
}

Rect clampRect(Rect r) {
  if (r.x < 0) { r.w += r.x; r.x = 0; }
  if (r.y < 0) { r.h += r.y; r.y = 0; }
  if (r.x + r.w > SCREEN_W) r.w = SCREEN_W - r.x;
  if (r.y + r.h > SCREEN_H) r.h = SCREEN_H - r.y;
  if (r.w < 1) r.w = 1;
  if (r.h < 1) r.h = 1;
  return r;
}

Rect expandRect(const Rect& r, int pad) {
  return clampRect({r.x - pad, r.y - pad, r.w + 2 * pad, r.h + 2 * pad});
}

/** Match Seeed_GFX updataPartial() — x/w are expanded to 8 px boundaries. */
Rect alignPartialBounds(Rect r) {
  r = clampRect(r);
  const int x0 = r.x & ~(EPD_PARTIAL_ALIGN - 1);
  const int x1 = (r.x + r.w + EPD_PARTIAL_ALIGN - 1) & ~(EPD_PARTIAL_ALIGN - 1);
  return {x0, r.y, x1 - x0, r.h};
}

void clearForPartial(const Rect& logical, int pad = 8) {
  const Rect aligned = alignPartialBounds(expandRect(logical, pad));
  epaper.fillRect(aligned.x, aligned.y, aligned.w, aligned.h, TFT_WHITE);
}

void clearForPartialAsymmetric(const Rect& logical, int padLeft, int padTop, int padRight, int padBottom) {
  const Rect expanded = clampRect({logical.x - padLeft, logical.y - padTop,
                                   logical.w + padLeft + padRight, logical.h + padTop + padBottom});
  const Rect aligned = alignPartialBounds(expanded);
  epaper.fillRect(aligned.x, aligned.y, aligned.w, aligned.h, TFT_WHITE);
}

void clearRect(const Rect& r) {
  clearForPartial(r, 8);
}

/** Partial updates disabled — full GC refresh via epaper.update() only. */
void pushPartial(const Rect& logical, int pad = 8) {
  (void)logical;
  (void)pad;
}

void formatEtaMins(char* buf, size_t len, int minutes) {
  if (minutes <= 0) {
    snprintf(buf, len, "Arriving");
  } else {
    snprintf(buf, len, "%2d min", minutes);
  }
}

void drawHeaderTextAt(int x, int centerY, const char* text) {
  epaper.setTextColor(TFT_BLACK, TFT_WHITE, true);
#ifdef USE_MONTSERRAT
  setFont(FontRole::Header);
  epaper.setTextPadding(0);
  epaper.setTextDatum(ML_DATUM);
  epaper.drawString(text, x, centerY);
#else
  epaper.setTextDatum(ML_DATUM);
  epaper.setTextSize(FS_HEADER);
  epaper.drawString(text, x, centerY);
#endif
}

int headerTextWidth(const char* text) {
#ifdef USE_MONTSERRAT
  setFont(FontRole::Header);
#else
  epaper.setTextSize(FS_HEADER);
#endif
  return epaper.textWidth(text);
}

int textWidthFor(FontRole role, const char* text) {
#ifdef USE_MONTSERRAT
  setFont(role);
#else
  epaper.setTextSize(textSizeFor(role));
#endif
  return epaper.textWidth(text);
}

void drawTextInRect(const Rect& r, const char* text, FontRole role, int baselineOffset = 24) {
  epaper.setTextColor(TFT_BLACK, TFT_WHITE, true);
#ifdef USE_MONTSERRAT
  setFont(role);
  epaper.setTextPadding(r.w + 8);
  epaper.setTextDatum(ML_DATUM);
  epaper.drawString(text, r.x, r.y + r.h / 2);
#else
  epaper.setTextDatum(TL_DATUM);
  epaper.setTextSize(textSizeFor(role));
  epaper.drawString(text, r.x, r.y + baselineOffset);
#endif
}

void drawHeaderTexts(const tm& timeinfo, const DashboardData& data) {
  char dateBuf[16];
  char dayBuf[16];
  char clockBuf[16];
  char batBuf[16];
  formatDate(dateBuf, sizeof(dateBuf), timeinfo);
  formatWeekday(dayBuf, sizeof(dayBuf), timeinfo);
  formatClock(clockBuf, sizeof(clockBuf), timeinfo);
  snprintf(batBuf, sizeof(batBuf), "%u%%", data.batteryPct);

  const int centerY = HEADER_Y + HEADER_H / 2;
  int x = BOARD_PAD + HEADER_LEFT_PAD;
  drawHeaderTextAt(x, centerY, dateBuf);
  x += headerTextWidth(dateBuf) + HEADER_PIPE_GAP;
  drawHeaderTextAt(x, centerY, "|");
  x += headerTextWidth("|") + HEADER_PIPE_GAP;
  drawHeaderTextAt(x, centerY, clockBuf);
  x += headerTextWidth(clockBuf) + HEADER_PIPE_GAP;
  drawHeaderTextAt(x, centerY, "|");
  x += headerTextWidth("|") + HEADER_PIPE_GAP;
  drawHeaderTextAt(x, centerY, dayBuf);

  Rect bat = headerBattery();
  const auto icon = assets::battery();
  const int iconY = bat.y + (bat.h - icon.height) / 2;
  drawAsset(bat.x, iconY, icon);
  const int textX = bat.x + icon.width + UI_ICON_TEXT_GAP;
  drawTextInRect({textX, bat.y, bat.w - (textX - bat.x), bat.h}, batBuf, FontRole::Header, 24);
  redrawHeaderRule();
}

void drawTextBoldInRect(const Rect& r, const char* text, FontRole role, int baselineOffset = 24) {
#ifdef USE_MONTSERRAT
  drawTextInRect(r, text, role, baselineOffset);
#else
  drawTextInRect(r, text, role, baselineOffset);
  drawTextInRect({r.x + 1, r.y, r.w, r.h}, text, role, baselineOffset);
#endif
}

void drawTextRightInRect(const Rect& r, const char* text, FontRole role) {
  epaper.setTextColor(TFT_BLACK, TFT_WHITE, true);
#ifdef USE_MONTSERRAT
  setFont(role);
  epaper.setTextPadding(r.w + 8);
  epaper.setTextDatum(MR_DATUM);
  epaper.drawString(text, r.x + r.w, r.y + r.h / 2);
#else
  epaper.setTextDatum(TR_DATUM);
  epaper.setTextSize(textSizeFor(role));
  epaper.drawString(text, r.x + r.w, r.y + 20);
#endif
}

void drawSensorRegion(const DashboardData& data) {
  char line[48];
  Rect r = sensorPanel();
  const int rowY = r.y + SECTION_TOP_PAD + SENSOR_ROW_Y_OFFSET;
  const int baseX = r.x + PANEL_PAD_LEFT;
  const int secondX = baseX + UI_ICON_SIZE + UI_ICON_TEXT_GAP + SENSOR_VALUE_W + SENSOR_ITEM_GAP;

  clearRect({r.x + PANEL_PAD_LEFT, r.y + SECTION_TOP_PAD, r.w - PANEL_PAD_LEFT - PANEL_PAD, r.h - SECTION_TOP_PAD - 8});

  drawTextInRect({baseX, r.y + SECTION_TOP_PAD, 120, 28}, "Bedroom", FontRole::Title, 18);

  const auto tempIcon = assets::temperature();
  drawAssetVCentered(baseX, rowY, SENSOR_ROW_H, tempIcon);
  snprintf(line, sizeof(line), "%.1f°C", data.sensorTempC);
  drawTextInRect({baseX + tempIcon.width + UI_ICON_TEXT_GAP, rowY, SENSOR_VALUE_W, SENSOR_ROW_H}, line, FontRole::SensorValue, 20);

  const auto humIcon = assets::humidity();
  drawAssetVCentered(secondX, rowY, SENSOR_ROW_H, humIcon);
  snprintf(line, sizeof(line), "%.1f%%", data.sensorHumidityPct);
  drawTextInRect({secondX + humIcon.width + UI_ICON_TEXT_GAP, rowY, SENSOR_VALUE_W, SENSOR_ROW_H}, line, FontRole::SensorValue, 20);

  redrawBodyDividers();
}

static void drawWeatherStats(int x, int y, int maxX, const DashboardData& data) {
  char buf[16];
  int cx = x;

  const auto drawStat = [&](const assets::MonoBitmap& icon, float value, bool tight) {
    drawAssetVCentered(cx, y, WEATHER_STATS_H, icon);
    snprintf(buf, sizeof(buf), "%.1f", value);
    const int gap = tight ? STAT_TIGHT_GAP : UI_ICON_TEXT_GAP;
    const int textX = cx + icon.width + gap;
    drawTextInRect({textX, y, maxX - textX, WEATHER_STATS_H}, buf, FontRole::Title, 18);
    cx = textX + textWidthFor(FontRole::Title, buf) + STAT_GROUP_GAP;
  };

  drawStat(assets::temperatureMin(), data.weatherMinC, true);
  drawStat(assets::temperatureMax(), data.weatherMaxC, true);
  drawStat(assets::rain(), data.weatherPrecipMm, false);
}

void drawTflRegion(const DashboardData& data) {
  const int tflTextX = ROUNDEL_X + asset_elizabeth_line_roundel_width + TFL_ROUNDEL_GAP;
  const int titleY = ROUNDEL_Y + 8;
  const int titleH = 28;
  const int headerClearH = TFL_LIST_TOP - (BODY_Y + SECTION_TOP_PAD);

  clearForPartial({tflTextX, BODY_Y + SECTION_TOP_PAD, SPLIT_X - tflTextX - TFL_PAD_RIGHT, headerClearH});

  drawRoundel();

  int tx = tflTextX;
  drawTextInRect({tx, titleY, 300, titleH}, data.tflLine, FontRole::Title, 18);
  tx += textWidthFor(FontRole::Title, data.tflLine);

  bool drawStaleIcon = data.apiOk && data.apiStale;
  const bool drawWarnIcon = data.apiOk && !drawStaleIcon &&
                            (data.tflDisruption || strcmp(data.tflStatus, "Good Service") != 0);

  if (drawStaleIcon || drawWarnIcon) {
    const auto icon = drawStaleIcon ? assets::stale() : assets::warning();
    tx += TFL_INLINE_GAP;
    if (drawStaleIcon) tx += TFL_STALE_X_OFFSET;
    drawAsset(tx, titleY + (titleH - icon.height) / 2 + TFL_TITLE_ICON_Y_OFFSET, icon);
  }

  char sub[80];
  snprintf(sub, sizeof(sub), "From %s · %s", data.tflStop, data.tflDirection);
  drawTextInRect({tflTextX, ROUNDEL_Y + 38, 300, 20}, sub, FontRole::Subtitle, 12);

  for (uint8_t i = 0; i < ETA_COUNT; i++) {
    char line[64];
    const int rowY = etaRowTextY(i);
    const Rect minsR = {ETA_MINS_X, rowY, ETA_MINS_W, ETA_LINE_H};

    if (data.apiOk && i < data.tflEtaCount) {
      snprintf(line, sizeof(line), "%u - %-16s", i + 1, data.tflEtas[i].destination);
      drawTextInRect({CONTENT_X, rowY, ETA_DEST_SLOT_W, ETA_LINE_H}, line, FontRole::Title, 18);
      formatEtaMins(line, sizeof(line), data.tflEtas[i].minutes);
      drawTextRightInRect(minsR, line, FontRole::Title);
    } else {
      snprintf(line, sizeof(line), "%u - N/A", i + 1);
      drawTextInRect({CONTENT_X, rowY, ETA_DEST_SLOT_W, ETA_LINE_H}, line, FontRole::Title, 18);
      drawTextRightInRect(minsR, "-- min", FontRole::Title);
    }

    drawDottedLine(CONTENT_X, etaDottedY(i), DOTTED_LINE_X1, etaDottedY(i));
  }

  redrawBodyDividers();
}

void drawWeatherRegion(const DashboardData& data) {
  Rect r = weatherPanel();
  const int contentTop = r.y + PANEL_PAD;
  const int contentH = r.h - 2 * PANEL_PAD;
  const int contentW = r.w - PANEL_PAD_LEFT - PANEL_PAD;
  const int blockH[] = {WEATHER_LABEL_H, WEATHER_MAIN_H, WEATHER_STATS_H};
  constexpr uint8_t BLOCK_COUNT = 3;

  const int yLabel = spaceEvenlyY(contentTop, contentH, blockH, BLOCK_COUNT, 0);
  const int yMain = spaceEvenlyY(contentTop, contentH, blockH, BLOCK_COUNT, 1);
  const int yStats = spaceEvenlyY(contentTop, contentH, blockH, BLOCK_COUNT, 2);
  const int iconX = r.x + PANEL_PAD_LEFT;
  const int mainTextX = iconX + WEATHER_ICON + WEATHER_ICON_GAP;
  const int textBlockY = yMain + (WEATHER_MAIN_H - WEATHER_HOURLY_TEXT_BLOCK_H) / 2;

  clearRect({r.x + PANEL_PAD_LEFT, contentTop, contentW, contentH});

  drawTextInRect({iconX, yLabel, contentW, WEATHER_LABEL_H},
                 data.weatherLocation, FontRole::Title, 18);

  if (!data.apiOk) {
    const int singleLineY = yMain + (WEATHER_MAIN_H - WEATHER_HOURLY_LINE_H) / 2;
    drawTextBoldInRect({mainTextX, singleLineY, r.x + PANEL_PAD_LEFT + contentW - mainTextX, WEATHER_HOURLY_LINE_H},
                       "Unavailable", FontRole::WeatherHourlyBold, 13);
    redrawBodyDividers();
    return;
  }

  const auto weatherIcon = assets::weatherIcon(data.weatherIcon);
  drawAssetVCentered(iconX, yMain, WEATHER_MAIN_H, weatherIcon);

  char current[64];
  if (isnan(data.weatherTempC) || isnan(data.weatherFeelsLikeC)) {
    snprintf(current, sizeof(current), "%s", data.weatherDescription);
    const int singleLineY = yMain + (WEATHER_MAIN_H - WEATHER_HOURLY_LINE_H) / 2;
    drawTextBoldInRect({mainTextX, singleLineY, r.x + PANEL_PAD_LEFT + contentW - mainTextX, WEATHER_HOURLY_LINE_H},
                       current, FontRole::WeatherHourlyBold, 13);
  } else {
    snprintf(current, sizeof(current), "%s", data.weatherDescription);
    drawTextBoldInRect({mainTextX, textBlockY, r.x + PANEL_PAD_LEFT + contentW - mainTextX, WEATHER_HOURLY_LINE_H},
                       current, FontRole::WeatherHourlyBold, 13);

    snprintf(current, sizeof(current), "%.1f°C / feels like %.1f°C", data.weatherTempC, data.weatherFeelsLikeC);
    drawTextInRect({mainTextX, textBlockY + WEATHER_HOURLY_LINE_H + WEATHER_HOURLY_TEXT_GAP,
                    r.x + PANEL_PAD_LEFT + contentW - mainTextX, WEATHER_HOURLY_LINE_H},
                   current, FontRole::WeatherHourly, 13);

    drawWeatherStats(iconX, yStats, r.x + PANEL_PAD_LEFT + contentW, data);
  }

  redrawBodyDividers();
}

void redrawAllRegions(const DashboardData& data, const tm& timeinfo) {
  drawHeaderDivider();
  drawRoundel();
  redrawTflDottedLines();
  drawHeaderTexts(timeinfo, data);
  drawSensorRegion(data);
  drawTflRegion(data);
  drawWeatherRegion(data);
}

}  // namespace

void finishInitialDraw() {
  // fullScreenRefresh() performs the initial epaper.update().
}

void fullScreenRefresh(const DashboardData& data, tm timeinfo) {
  partialEnabled = false;
  epaper.fillScreen(TFT_WHITE);
  redrawAllRegions(data, timeinfo);
  epaper.update();
}

Rect regionBounds(RefreshRegion region) {
  switch (region) {
    case RefreshRegion::Clock: return headerClock();
    case RefreshRegion::Date: return {headerDate().x, headerDate().y, headerDate().w + headerWeekday().w + 20, headerDate().h};
    case RefreshRegion::Battery: return headerBattery();
    case RefreshRegion::Sensors: return sensorPanel();
    case RefreshRegion::Tfl: return tflPanel();
    case RefreshRegion::Weather: return weatherPanel();
  }
  return {0, 0, 0, 0};
}

void begin() {
  partialEnabled = false;
  epaper.begin();
  epaper.fillScreen(TFT_WHITE);
  epaper.setTextColor(TFT_BLACK, TFT_WHITE, true);
}

void drawStaticChrome() {
  drawHeaderDivider();
  drawRoundel();
  redrawTflDottedLines();
}

}  // namespace display

#endif
