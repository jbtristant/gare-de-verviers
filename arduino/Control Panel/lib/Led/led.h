#pragma once

#include <Arduino.h>

class Led {
    public:
        explicit Led(uint8_t pin);
        void begin();
        void toggle();
        void turnOn();
        void turnOff();
        bool getState();

    private:
        uint8_t m_pin;
        bool m_state;
};
