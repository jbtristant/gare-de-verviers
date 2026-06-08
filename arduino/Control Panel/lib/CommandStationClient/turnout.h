#pragma once

#include <Arduino.h>

#include "types.h"

class CommandStationClient;

enum class TurnoutState : uint8_t { Close = 0, Throw = 1, eXamine = 2, Undefined = 3};


class Turnout {
    public:
    Turnout()
    : m_commandStationClient(nullptr), m_id(0), m_address(0), m_state(TurnoutState::Undefined) {
        m_name[0] = '\0';
    }
    
    void initialize(CommandStationClient* client, uint16_t id, TurnoutState state) {
        m_commandStationClient = client;
        m_id = id;
        m_state = state;
    }

    CommandStationClient* getCommandStationClient();

    void setId(uint16_t id) {
        m_id = id;
    }

    uint16_t getId() {
        return m_id;
    }

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

    void setState(TurnoutState state);

    void setStatePrivate(TurnoutState state) {
        m_state = state;
    }
    
    TurnoutState getState() {
        return m_state;
    }


private:
    CommandStationClient *m_commandStationClient;
    uint16_t m_id = 0; // id is the id in myAutomation.h
    uint16_t m_address = 0; // address is the physical address on the board
    char m_name[30];
    TurnoutState m_state = TurnoutState::Close;
};
