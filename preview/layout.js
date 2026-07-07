/** Layout constants — keep in sync with arduino/display_layout.cpp */
export const LAYOUT = {
  screenW: 800,
  screenH: 480,
  boardPad: 20,
  headerLeftPad: 8,
  sectionTopPad: 28,
  headerH: 56,
  splitX: 464, // 58% of 800px screen
  sensorH: 140,
  contentX: 32, // boardPad + tflPadLeft
  roundelHeight: 64,
  roundelEtaGap: 16,
  tflPadLeft: 12,
  tflPadRight: 28,
  tflRoundelGap: 12,
  panelPadLeft: 20,
  etaTopPad: 26,
  etaFirstTopPad: 12,
  etaBottomPad: 18,
  etaDotGap: 8,
  etaDotSize: 2,
  etaCount: 4,
  /** Line box for ETA/title type — keep in sync with TYPO.title */
  get etaLineH() { return TYPO.title; },
  get etaFirstRowH() { return this.etaFirstTopPad + this.etaLineH + this.etaBottomPad + 1; },
  get etaRowH() { return this.etaTopPad + this.etaLineH + this.etaBottomPad + 1; },
  get bodyY() { return this.boardPad + this.headerH; },
  get roundelTop() { return this.bodyY + this.sectionTopPad; },
  get etaTop() { return this.roundelTop + this.roundelHeight + this.roundelEtaGap; },
  get dottedYs() {
    const t = this.etaTop;
    const lineEnd = (topPad, rowOffset) => t + rowOffset + topPad + this.etaLineH + this.etaBottomPad;
    const ys = [lineEnd(this.etaFirstTopPad, 0)];
    for (let i = 1; i < this.etaCount; i++) {
      const rowOffset = this.etaFirstRowH + (i - 1) * this.etaRowH;
      ys.push(lineEnd(this.etaTopPad, rowOffset));
    }
    return ys;
  },
  /** First grid column width inside board padding — splitX minus left pad */
  get splitColW() { return this.splitX - this.boardPad; },
};

/**
 * font-size = CSS em square / VLW point size (matches design spec).
 * Montserrat cap height ≈ 0.72× font-size (18px → ~13px caps) — normal, not a bug.
 * Use lineHeight for vertical spacing between elements, not cap height.
 */
export const TYPO = {
  header: 30,
  title: 24,       // section titles, ETAs, daily weather
  subtitle: 16,    // TfL subtitle, status
  sensor: 26,      // bedroom readings
  weatherHourly: 17,
  weatherDaily: 24,
  lineHeight: {
    header: 30,
    title: 24,
    subtitle: 16,
    sensor: 26,
    weatherHourly: 17,
    weatherDaily: 24,
  },
};

/** title = section labels + ETAs + daily stats; weatherHourly = condition + temp */

/**
 * CSS space-evenly port (Arduino has no flexbox — compute Y manually):
 *   gap = (containerH - sum(blockHeights)) / (blockCount + 1)
 * See spaceEvenlyY() in arduino/display_layout.cpp
 */
export const WEATHER_BLOCKS = {
  pad: 16,
  padLeft: 20,
  icon: 64,
  iconGap: 6,
  iconTextGap: 4,
  /** Min/max thermometers sit flush against values; rain keeps iconTextGap */
  statTightGap: 0,
  labelH: 28,
  mainH: 64,
  hourlyLineH: 17,
  hourlyTextGap: 6,
  statsH: 36,
  iconUi: 36,
  iconBattery: 30,
};

const P = LAYOUT.boardPad;
const BY = LAYOUT.bodyY;

export const REGIONS = {
  header: { x: 0, y: P, w: 800, h: LAYOUT.headerH },
  date: { x: P + LAYOUT.headerLeftPad, y: P + 8, w: 180, h: 40 },
  clock: { x: 254, y: P + 4, w: 220, h: 48 },
  weekday: { x: 504, y: P + 8, w: 140, h: 40 },
  battery: { x: 670, y: P + 8, w: 110, h: 40 },
  tfl: { x: 0, y: BY, w: LAYOUT.splitX, h: LAYOUT.screenH - BY - P },
  sensors: { x: LAYOUT.splitX, y: BY, w: LAYOUT.screenW - LAYOUT.splitX, h: LAYOUT.sensorH },
  weather: { x: LAYOUT.splitX, y: BY + LAYOUT.sensorH, w: LAYOUT.screenW - LAYOUT.splitX, h: LAYOUT.screenH - BY - LAYOUT.sensorH - P },
};
