#pragma once

#include <Arduino.h>

#include "types.h"

enum class TrackType { Both, Main, Prog, Join, None };
enum class TrackMode { Prog, Main, Main_inv, Main_a, Dc, Dcx, None };

// Helper pour convertir l'enum en chaîne de caractères (Strictement compatible
// C++11)
constexpr const char *trackTypeToCString(TrackType type) {
  return (type == TrackType::Both)      ? "BOTH"
         : (type == TrackType::Main)    ? "MAIN"
         : (type == TrackType::Prog)    ? "PROG"
         : (type == TrackType::Join)    ? "JOIN"
                                        : "NONE";
}

constexpr const char *trackModeToCString(TrackMode mode) {
  return (mode == TrackMode::Prog)       ? "PROG"
         : (mode == TrackMode::Main)     ? "MAIN"
         : (mode == TrackMode::Main_inv) ? "MAIN_INV"
         : (mode == TrackMode::Main_a)   ? "MAIN A"
                                         : // Avec l'espace requis par DCC-EX
             (mode == TrackMode::Dc) ? "DC"
         : (mode == TrackMode::Dcx)  ? "DCX"
                                     : "NONE";
}


constexpr const char *SpeedstepToCString(Speedstep speedstep) {
  return (speedstep == Speedstep::Speed_28)    ? "SPEED28"
         : (speedstep == Speedstep::Speed_128) ? "SPEED128"
                                               : "SPEED128";
}


class CommandStationClient;

class Track {
    public:
    Track()
    : m_commandStationClient(nullptr), 
      m_name(' '),
      m_type(TrackType::None), 
      m_mode(TrackMode::None), 
      m_power(OnOff::Unknown),
      m_current(0),
      m_maxCurrent(0) {

    }
    
    void initialize(CommandStationClient* client, char name, const TrackType value) {
        m_commandStationClient = client;
        m_name = name;
        m_type = value;
    }

    void setName(const char name) {
        m_name = name;
    }

    const char getName() {
        return m_name;
    }

    void setTypePrivate(const TrackType value) {
        m_type = value;
    }

    TrackType getType() {
        return m_type;
    }

    void setMode(const TrackMode mode);

    void setModePrivate(const TrackMode mode) {
        m_mode = mode;
    }

    const TrackMode getMode() {
        return m_mode;
    }

    void setPower(const OnOff value);
    void setPowerWithName(const OnOff value);
    void setPowerWithType(const OnOff value);
    void setPowerPrivate(const OnOff value);

    const OnOff getPower() {
        return m_power;
    }

    void setCurrent(const uint16_t current) {
        m_current = current;
    }

    const uint16_t getCurrent() {
        return m_current;
    }
    
    void setMaxCurrent(const uint16_t current) {
        m_maxCurrent = current;
    }

    const uint16_t getMaxCurrent() {
        return m_maxCurrent;
    }

private:
    CommandStationClient *m_commandStationClient;
    char m_name;
    TrackType m_type;
    TrackMode m_mode;
    OnOff m_power;
    uint16_t m_current;
    uint16_t m_maxCurrent;
};