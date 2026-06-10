#pragma once

#include <Arduino.h>

enum class Direction : uint8_t { Forward = 1, Reverse = 0 };

class CommandStationClient;

class Locomotive
{
  public:
    Locomotive();

    void initialize(CommandStationClient *client, uint16_t address);

    void askLocoInfo();

    void setAddress(uint16_t address) { m_address = address; }
    uint16_t getAddress() { return m_address; }

    void setName(const char *name);

    const char *getName();

    void setFunctions(const char *functions);

    void setSpeed(int8_t speed, Direction direction);

    void setSpeedPrivate(int8_t speed, Direction direction);

    int8_t getSpeed();

    Direction getDirection();

  private:
    CommandStationClient *m_commandStationClient;
    uint16_t m_address = 0;
    char m_name[30];
    char m_functions[50];
    int8_t m_speed = 0;
    Direction m_direction;
};
