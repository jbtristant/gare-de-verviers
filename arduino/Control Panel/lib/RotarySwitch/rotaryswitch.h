/*
        Created by Jean-Benoit Tristant, May 11, 2026
*/

#pragma once

#include <Arduino.h>
#include <semphr.h>

class RotarySwitch
{
  public:
    RotarySwitch(uint8_t pinSwitch, uint8_t pinClk, uint8_t pinDt, uint8_t id);
    void setCallbackClicked(void (*pushButtonCallbackClicked)(uint8_t id));
    void setRotaryCallbackChanged(void (*rotaryCallbackChanged)(uint8_t id, bool clockWize));
    void initialize(Stream *logStream, SemaphoreHandle_t xLogStreamSemaphore);
    static void TaskReadPinsStatic(void *pvParameters);

  private:
    void taskReadPins();
    void (*m_pushButtonCallbackClicked)(uint8_t);
    void (*m_rotaryCallbackChanged)(uint8_t, bool);

    Stream *m_logStream;
    SemaphoreHandle_t m_xLogStreamSemaphore;

    uint8_t m_pinSwitch, m_pinClk, m_pinDt;
    uint8_t m_id;
    volatile int m_statePinSwitch, m_statePinClk, m_statePinDt;
    int m_lastStatePinSwitch, m_lastStatePinClk, m_lastStatePinDt;
    unsigned long m_time;
};