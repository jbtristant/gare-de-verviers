#pragma once

#include <Arduino.h>

enum class Direction : uint8_t { Forward = 1, Reverse = 0 };

class CommandStationClient;

class Locomotive {
    public:
    Locomotive()
    : m_commandStationClient(nullptr), m_address(0) {
        m_name[0] = '\0';
        m_functions[0] = '\0';
    }
    
    void initialize(CommandStationClient* client, uint16_t address) {
        m_commandStationClient = client;
        m_address = address;
    }

    void askLocoInfo();

    void setAddress(uint16_t address) {
        m_address = address;
    }
    uint16_t getAddress() {
        return m_address;
    }

    void setName(const char* name) {
        strncpy(m_name, name, sizeof(m_name) - 1);
        m_name[sizeof(m_name) - 1] = '\0';
    }

    const char* getName() {
        return m_name;
    }

    void setFunctions(const char* functions) {
        strncpy(m_functions, functions, sizeof(m_functions) - 1);
        m_functions[sizeof(m_functions) - 1] = '\0';
    }

    void setSpeed(int8_t speed, Direction direction);

    void setSpeedPrivate(int8_t speed, Direction direction);
    
    int8_t getSpeed() {
        return m_speed;
    }

    Direction getDirection() {
        return m_direction;
    }

private:
    CommandStationClient *m_commandStationClient;
    uint16_t m_address = 0;
    char m_name[30];
    char m_functions[50];
    int8_t m_speed = 0;
    Direction m_direction;
};
