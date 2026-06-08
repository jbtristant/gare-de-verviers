#include <Arduino.h>

#include "locomotive.h"
#include "commandstationclient.h"


void Locomotive::askLocoInfo() {
    m_commandStationClient->askLocoInfo(m_address);
}

void Locomotive::setSpeed(int8_t speed, Direction direction) {
    m_commandStationClient->setLocoSpeed(m_address, speed, direction);
}

void Locomotive::setSpeedPrivate(int8_t speed, Direction direction) {
    Serial.print(F("SetSpeedPrivate: "));
    Serial.print(speed);
    Serial.print(F(" direction "));
    (direction == Direction::Forward) ? Serial.println(F("Forward")) : Serial.println(F("Reverse"));
    m_speed = speed;
    m_direction = direction;
}