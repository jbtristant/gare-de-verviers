/*
	Created by Jean-Benoit Tristant, May 11, 2026
*/

#include "rotaryswitch.h"
#include "Arduino.h"
#include "pins_arduino.h"


#if defined(__AVR_ATmega2560__)
    // Code spécifique à l'Arduino Mega 2560
    static inline uint8_t fastDigitalRead(uint8_t pin) {
        // Pin 2 (PE4), Pin 3 (PE5), Pin 5 (PE3) -> Toutes sur le Port E
        if (pin == 2) return (PINE & (1 << 4)) ? HIGH : LOW;
        if (pin == 3) return (PINE & (1 << 5)) ? HIGH : LOW;
        if (pin == 5) return (PINE & (1 << 3)) ? HIGH : LOW;
        return digitalRead(pin); // Sécurité
    }

#elif defined(__AVR_ATmega328P__)
    // Code spécifique à l'Arduino Uno / Nano / Pro Mini
    static inline uint8_t fastDigitalRead(uint8_t pin) {
        if (pin < 8) return (PIND & (1 << pin)) ? HIGH : LOW;
        if (pin < 14) return (PINB & (1 << (pin - 8))) ? HIGH : LOW;
        return LOW; // Sécurité pour les autres pins
    }

#else
    // Code générique ou pour d'autres cartes
    return digitalRead(pin);
#endif

RotarySwitch* RotarySwitch::s_instance = nullptr;

RotarySwitch::RotarySwitch(uint8_t pinSwitch, uint8_t pinClk, uint8_t pinDt, uint8_t id, bool interceptMode):
    m_pinSwitch(pinSwitch), m_pinClk(pinClk), m_pinDt(pinDt), m_id(id), m_interceptMode(interceptMode)
{
    s_instance = this;
}

void RotarySwitch::begin() {
    pinMode(m_pinSwitch, INPUT);
    pinMode(m_pinClk, INPUT);
    pinMode(m_pinDt, INPUT);

    delay(200);

    m_lastStatePinSwitch = digitalRead(m_pinSwitch);
    m_lastStatePinClk = digitalRead(m_pinClk);
    m_lastStatePinDt = digitalRead(m_pinDt);

    if (m_interceptMode) {
        attachInterrupt(digitalPinToInterrupt(m_pinClk), RotarySwitch::isrClkBridge, CHANGE);
        attachInterrupt(digitalPinToInterrupt(m_pinSwitch), RotarySwitch::isrSwBridge, CHANGE);
    }
}

void RotarySwitch::process() 
{
    if (m_time + 5 < millis()) {
        if (!m_interceptMode) {
            m_statePinClk = digitalRead(m_pinClk);
            m_statePinDt  = digitalRead(m_pinDt);
            m_statePinSwitch = digitalRead(m_pinSwitch);
        }
            
        if((m_statePinClk == HIGH) && (m_statePinDt == LOW) && (m_lastStatePinClk == LOW) && (m_lastStatePinDt == HIGH)) 
        {
            m_rotaryCallbackChanged(m_id, true);
        }

        if ((m_statePinClk == HIGH) && (m_statePinDt == HIGH) && (m_lastStatePinClk == LOW) && (m_lastStatePinDt == LOW)) {
            m_rotaryCallbackChanged(m_id, false);
        }

        if (m_lastStatePinSwitch == LOW && m_statePinSwitch == HIGH) {
            m_pushButtonCallbackClicked(m_id);
            m_rotaryCallbackChanged(m_id, true);
        }

        m_lastStatePinClk = m_statePinClk;
        m_lastStatePinDt = m_statePinDt;
        m_lastStatePinSwitch = m_statePinSwitch;
        m_time = millis();
    }
}

void RotarySwitch::setCallbackClicked(void (*pushButtonCallbackClicked)(uint8_t id))
{
    m_pushButtonCallbackClicked = pushButtonCallbackClicked;
}

void RotarySwitch::setRotaryCallbackChanged(void (*rotaryCallbackChanged)(uint8_t id, bool clockWize))
{
    m_rotaryCallbackChanged = rotaryCallbackChanged;
}

void RotarySwitch::isrClkBridge()
{
    if (s_instance != nullptr) {
        //Serial.println(F("Read Clk and DT state"));
        s_instance->m_statePinClk = fastDigitalRead(s_instance->m_pinClk);
        s_instance->m_statePinDt = fastDigitalRead(s_instance->m_pinDt);
    }
}

void RotarySwitch::isrSwBridge()
{
    if (s_instance != nullptr) {
        s_instance->m_statePinSwitch = fastDigitalRead(s_instance->m_pinSwitch);
    }
}
