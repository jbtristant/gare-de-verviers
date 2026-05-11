/*
	Created by Jean-Benoit Tristant, Avril 23, 2017
*/
#pragma once

#include <Arduino.h>

class PushButton
{
  public:
    PushButton(uint8_t pin, uint8_t id);
    void setCallbackClicked(void (*pushButtonCallbackClicked)(uint8_t));
    void begin();
    void process();

  private:
    void (*_pushButtonCallbackClicked)(uint8_t);
    uint8_t m_pin;
    uint8_t m_id;
    int m_state;
    int m_lastState;
    unsigned long m_time;
};
