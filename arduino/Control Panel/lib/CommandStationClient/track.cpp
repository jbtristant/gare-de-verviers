#include <Arduino.h>

#include "track.h"

#include "commandstationclient.h"


void Track::setMode(const TrackMode mode) 
{
    m_commandStationClient->configureTrackManager(m_name, mode);
}

void Track::setPower(const OnOff value) 
{
   setPowerWithName(value);
}

void Track::setPowerWithName(const OnOff value)
{
    m_commandStationClient->powerTrack(value, m_name);
}

void Track::setPowerWithType(const OnOff value)
{
    m_commandStationClient->powerTrack(value, m_type);
}

void Track::setPowerPrivate(const OnOff value) 
{
    Serial.print(F("Track::SetPowerPrivate "));
    Serial.print(m_name);
    Serial.print(F(" is "));
    Serial.println(onOffToCString(value));
    m_power = value;
}