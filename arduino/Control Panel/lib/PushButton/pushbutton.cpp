/*
  Created by Jean-Benoit Tristant, Avril 23, 2017.
*/

#include "pushbutton.h"

PushButton::PushButton(uint8_t pin, uint8_t id)
{
  m_pin = pin;
  m_id = id;
  m_state = 1;
  m_lastState = 1;
  m_time = 0;
}

void PushButton::begin() 
{
  pinMode(m_pin, INPUT_PULLUP);
}

void PushButton::process()
{
  if (m_time + 50 < millis()) {
    m_state = digitalRead(m_pin);
    if (m_state == 0 && m_lastState == 1) {
      _pushButtonCallbackClicked(m_id);
    }
    m_lastState = m_state;
    m_time = millis();
  }
}

void PushButton::setCallbackClicked(void (*pushButtonCallbackClicked)(uint8_t))
{
  _pushButtonCallbackClicked = pushButtonCallbackClicked;
}

