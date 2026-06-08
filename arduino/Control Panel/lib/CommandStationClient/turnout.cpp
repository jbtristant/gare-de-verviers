#include <Arduino.h>

#include "commandstationclient.h"

#include "turnout.h"

void Turnout::setState(TurnoutState state) 
{
    if (m_commandStationClient == nullptr) return;

    Serial.print(F("Set turnout "));
    Serial.print(m_name);
    Serial.print(F(" ("));
    Serial.print(m_id);
    Serial.print(F(") "));
    Serial.println(uint8_t(state));
    m_commandStationClient->ThrowCloseTurnout(m_id, state);
}

CommandStationClient* Turnout::getCommandStationClient()
{
    return m_commandStationClient;
}