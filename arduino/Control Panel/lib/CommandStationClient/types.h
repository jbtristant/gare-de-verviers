#pragma once

#include <Arduino.h>


enum class OnOff : uint8_t { Off = 0, On = 1, Unknown = 2 };

enum class Speedstep : uint8_t { Speed_28, Speed_128 };
enum class Momentum : uint8_t { Linear, Power };

constexpr const char *onOffToCString(OnOff value) {
  return (value == OnOff::Off)   ? "Off"
         : (value == OnOff::On)  ? "On"
                                 : "Unknwon";
}