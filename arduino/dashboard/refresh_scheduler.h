#pragma once

#include <stdint.h>

enum class RefreshRegion : uint8_t {
  Clock,
  Date,
  Battery,
  Sensors,
  Tfl,
  Weather,
};

struct RefreshTask {
  RefreshRegion region;
  uint32_t intervalMs;
  uint32_t lastRunMs;
  bool due(uint32_t now) const {
    return now - lastRunMs >= intervalMs;
  }
};

struct RefreshScheduler {
  RefreshTask tasks[6];
  int lastDay = -1;
  uint32_t lastStaticRefreshMs = 0;
  uint32_t lastFullRefreshMs = 0;

  void begin();
  bool clockDue(uint32_t now) const;
  bool tflDue(uint32_t now) const;
  bool periodicDue(uint32_t now) const;
  bool sensorsDue(uint32_t now) const;
  bool weatherDue(uint32_t now) const;
  bool dateDue(uint32_t now, int dayOfYear);
  bool staticRefreshDue(uint32_t now) const;
  bool fullRefreshDue(uint32_t now) const;
  RefreshRegion nextDueRegion(uint32_t now, int dayOfYear);
  void markRan(RefreshRegion region, uint32_t now);
  void markStaticRefresh(uint32_t now);
  void markFullRefresh(uint32_t now);
  uint32_t msUntilNext(uint32_t now) const;
};
