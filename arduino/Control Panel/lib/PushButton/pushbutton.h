/*
	Created by Jean-Benoit Tristant, Avril 23, 2017
*/
#pragma once

#include <Arduino.h>

class PushButton
{
  public:
    PushButton(uint8_t pin, int id);
    void setCallbackClicked(void (*pushButtonCallbackClicked)(int));
    void begin();
    void process();
    void xProcess();

  private:
    void (*_pushButtonCallbackClicked)(int);
    uint8_t m_pin;
    int m_id;
    int m_state;
    int m_lastState;
    unsigned long m_time;
};
