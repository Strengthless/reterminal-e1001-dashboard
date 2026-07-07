#pragma once

#include <Arduino.h>
#include <cstdarg>
#include <cstdio>

#ifndef DASHBOARD_DIAGNOSTICS
#define DASHBOARD_DIAGNOSTICS 0
#endif

namespace diag {

// reTerminal E1001 carrier USB-UART (GPIO43 TX / GPIO44 RX @ 115200).
constexpr int SERIAL_RX = 44;
constexpr int SERIAL_TX = 43;

inline void begin() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
#if DASHBOARD_DIAGNOSTICS
  Serial1.println("[diag] serial ready");
#endif
}

inline void print(const char* message) {
#if DASHBOARD_DIAGNOSTICS
  Serial1.print(message);
  if (Serial) Serial.print(message);
#endif
}

inline void println(const char* message) {
#if DASHBOARD_DIAGNOSTICS
  Serial1.println(message);
  if (Serial) Serial.println(message);
#endif
}

inline void printf(const char* fmt, ...) {
#if DASHBOARD_DIAGNOSTICS
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  println(buf);
#endif
}

}  // namespace diag
