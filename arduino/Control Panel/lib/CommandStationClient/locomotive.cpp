#include <Arduino.h>

#include "commandstationclient.h"
#include "locomotive.h"

Locomotive::Locomotive()
    : m_commandStationClient(nullptr), m_address(0), m_speed(0),
      m_direction(Direction::Forward)
{
    m_name[0] = '\0';
    m_functions[0] = '\0';
}

void Locomotive::initialize(CommandStationClient *client, uint16_t address)
{
    m_commandStationClient = client;
    m_address = address;
}

void Locomotive::askLocoInfo()
{
    m_commandStationClient->askLocoInfo(m_address);
}

void Locomotive::setName(const char *name)
{
    strncpy(m_name, name, sizeof(m_name) - 1);
    m_name[sizeof(m_name) - 1] = '\0';
}

const char *Locomotive::getName() { return m_name; }

void Locomotive::setFunctions(const char *functions)
{
    strncpy(m_functions, functions, sizeof(m_functions) - 1);
    m_functions[sizeof(m_functions) - 1] = '\0';
}

void Locomotive::setSpeed(int8_t speed, Direction direction)
{
    m_commandStationClient->setLocoSpeed(m_address, speed, direction);
}

void Locomotive::setSpeedPrivate(int8_t speed, Direction direction)
{
    Serial.print(F("SetSpeedPrivate: "));
    Serial.print(speed);
    Serial.print(F(" direction "));
    (direction == Direction::Forward) ? Serial.println(F("Forward"))
                                      : Serial.println(F("Reverse"));
    m_speed = speed;
    m_direction = direction;
}

int8_t Locomotive::getSpeed() { return m_speed; }

Direction Locomotive::getDirection() { return m_direction; }
