#include <Arduino.h>
#include "led.h"

Led::Led(uint8_t pin) : m_pin(pin), m_state(false) {}

void Led::begin() {
    pinMode(m_pin, OUTPUT);
}

void Led::toggle() {
    m_state = !m_state;
    digitalWrite(m_pin, m_state);
}

void Led::turnOn() {
    if (!m_state) {
        m_state = true;
        digitalWrite(m_pin, m_state);
    }
}

void Led::turnOff() {
    if (m_state) {
        m_state = false;
        digitalWrite(m_pin, m_state);
    }
}

bool Led::getState() {
    return m_state;
}