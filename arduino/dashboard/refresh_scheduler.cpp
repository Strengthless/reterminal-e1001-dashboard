#include "refresh_scheduler.h"

#include "config.h"

namespace {
// Re-draw infrequently updated regions so they don't fade next to the 1 Hz clock.
constexpr uint32_t INTERVAL_STATIC_REFRESH_MS = 5 * 60 * 1000;
// Full panel refresh clears accumulated partial-refresh ghosting.
constexpr uint32_t INTERVAL_FULL_REFRESH_MS = 15 * 60 * 1000;
}  // namespace

void RefreshScheduler::begin() {
  tasks[0] = {RefreshRegion::Clock, INTERVAL_CLOCK_MS, 0};
  tasks[1] = {RefreshRegion::Date, 0, 0};
  tasks[2] = {RefreshRegion::Battery, INTERVAL_SENSORS_MS, 0};
  tasks[3] = {RefreshRegion::Sensors, INTERVAL_SENSORS_MS, 0};
  tasks[4] = {RefreshRegion::Tfl, INTERVAL_TFL_MS, 0};
  tasks[5] = {RefreshRegion::Weather, INTERVAL_WEATHER_MS, 0};
  lastDay = -1;
}

bool RefreshScheduler::clockDue(uint32_t now) const {
  return tasks[0].due(now);
}

bool RefreshScheduler::dateDue(uint32_t now, int dayOfYear) {
  (void)now;
  if (lastDay < 0) {
    lastDay = dayOfYear;
    return true;
  }
  if (dayOfYear != lastDay) {
    lastDay = dayOfYear;
    return true;
  }
  return false;
}

bool RefreshScheduler::staticRefreshDue(uint32_t now) const {
  return now - lastStaticRefreshMs >= INTERVAL_STATIC_REFRESH_MS;
}

bool RefreshScheduler::fullRefreshDue(uint32_t now) const {
  return now - lastFullRefreshMs >= INTERVAL_FULL_REFRESH_MS;
}

RefreshRegion RefreshScheduler::nextDueRegion(uint32_t now, int dayOfYear) {
  if (dateDue(now, dayOfYear)) {
    return RefreshRegion::Date;
  }

  for (const RefreshTask& task : tasks) {
    if (task.region == RefreshRegion::Date) continue;
    if (task.due(now)) return task.region;
  }

  return RefreshRegion::Clock;
}

void RefreshScheduler::markRan(RefreshRegion region, uint32_t now) {
  for (RefreshTask& task : tasks) {
    if (task.region == region) {
      task.lastRunMs = now;
      return;
    }
  }
}

void RefreshScheduler::markStaticRefresh(uint32_t now) {
  lastStaticRefreshMs = now;
}

void RefreshScheduler::markFullRefresh(uint32_t now) {
  lastFullRefreshMs = now;
}

uint32_t RefreshScheduler::msUntilNext(uint32_t now) const {
  uint32_t minMs = INTERVAL_CLOCK_MS;

  if (lastStaticRefreshMs != 0) {
    const uint32_t staticElapsed = now - lastStaticRefreshMs;
    if (staticElapsed >= INTERVAL_STATIC_REFRESH_MS) {
      minMs = 0;
    } else {
      const uint32_t staticRemaining = INTERVAL_STATIC_REFRESH_MS - staticElapsed;
      if (staticRemaining < minMs) minMs = staticRemaining;
    }
  }

  if (lastFullRefreshMs != 0) {
    const uint32_t fullElapsed = now - lastFullRefreshMs;
    if (fullElapsed >= INTERVAL_FULL_REFRESH_MS) {
      minMs = 0;
    } else {
      const uint32_t fullRemaining = INTERVAL_FULL_REFRESH_MS - fullElapsed;
      if (fullRemaining < minMs) minMs = fullRemaining;
    }
  }

  for (const RefreshTask& task : tasks) {
    if (task.intervalMs == 0) continue;

    const uint32_t elapsed = now - task.lastRunMs;
    if (elapsed >= task.intervalMs) return 0;

    const uint32_t remaining = task.intervalMs - elapsed;
    if (remaining < minMs) minMs = remaining;
  }

  return minMs;
}
