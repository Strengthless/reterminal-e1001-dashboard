#pragma once

// Must be included before any #ifdef EPAPER_ENABLE in sketch translation units.
// BOARD_SCREEN_COMBO selects Setup520 (reTerminal E1001) which defines EPAPER_ENABLE.

#include "driver.h"
#include <TFT_eSPI.h>

#if !defined(EPAPER_ENABLE)
#error "EPAPER_ENABLE not defined — use BOARD_SCREEN_COMBO 520 in driver.h and Seeed_GFX setup 520"
#endif

/**
 * Threshold smooth-font RGB565 blends to 1-bit before pushing partial updates.
 * Keeps setTextColor(TFT_BLACK, TFT_WHITE) for correct VLW alpha blending.
 */
class EPaper1Bit : public EPaper {
public:
  using EPaper::EPaper;

  void drawPixel(int32_t x, int32_t y, uint32_t color) override {
    if (_created && _bpp == 1) {
      TFT_eSprite::drawPixel(x, y, colorToMono(color));
      return;
    }
    EPaper::drawPixel(x, y, color);
  }

private:
  static uint8_t luminance565(uint32_t color) {
    const uint8_t r = ((color >> 11) & 0x1F) << 3;
    const uint8_t g = ((color >> 5) & 0x3F) << 2;
    const uint8_t b = (color & 0x1F) << 3;
    return static_cast<uint8_t>((r * 76 + g * 150 + b * 29) >> 8);
  }

  static uint8_t colorToMono(uint32_t color) {
    if (color == 0x0000) return 0;
    if (color == 0xFFFF) return 1;
    if (color <= 1) return static_cast<uint8_t>(color);
    return luminance565(color) < 130 ? 0u : 1u;
  }
};
