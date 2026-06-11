/*
        Created by Jean-Benoit Tristant, May 11, 2026
*/

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#include "rotaryswitch.h"

#if defined(__AVR_ATmega2560__)
// Code spécifique à l'Arduino Mega 2560
static inline uint8_t fastDigitalRead(uint8_t pin)
{
    // Pin 2 (PE4), Pin 3 (PE5), Pin 5 (PE3) -> Toutes sur le Port E
    if (pin == 2)
        return (PINE & (1 << 4)) ? HIGH : LOW;
    if (pin == 3)
        return (PINE & (1 << 5)) ? HIGH : LOW;
    if (pin == 5)
        return (PINE & (1 << 3)) ? HIGH : LOW;
    return digitalRead(pin); // Sécurité
}

#elif defined(__AVR_ATmega328P__)
// Code spécifique à l'Arduino Uno / Nano / Pro Mini
static inline uint8_t fastDigitalRead(uint8_t pin)
{
    if (pin < 8)
        return (PIND & (1 << pin)) ? HIGH : LOW;
    if (pin < 14)
        return (PINB & (1 << (pin - 8))) ? HIGH : LOW;
    return LOW; // Sécurité pour les autres pins
}

#else
// Code générique ou pour d'autres cartes
return digitalRead(pin);
#endif

const int8_t ENCODER_STATES[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

RotarySwitch::RotarySwitch(uint8_t pinSwitch, uint8_t pinClk, uint8_t pinDt, uint8_t id)
    : m_pushButtonCallbackClicked(nullptr), m_rotaryCallbackChanged(nullptr), m_logStream(nullptr), m_xLogStreamSemaphore(nullptr), m_pinSwitch(pinSwitch),
      m_pinClk(pinClk), m_pinDt(pinDt), m_id(id)
{
}

void RotarySwitch::initialize(Stream *logStream, SemaphoreHandle_t xLogStreamSemaphore)
{
    pinMode(m_pinSwitch, INPUT);
    pinMode(m_pinClk, INPUT);
    pinMode(m_pinDt, INPUT);

    m_logStream = logStream;
    m_xLogStreamSemaphore = xLogStreamSemaphore;

    xTaskCreate(RotarySwitch::TaskReadPinsStatic, "RotarySwitch", 200, this, 3, NULL);
}

void RotarySwitch::TaskReadPinsStatic(void *pvParameters)
{
    RotarySwitch *instance = static_cast<RotarySwitch *>(pvParameters);

    instance->taskReadPins();
}

void RotarySwitch::taskReadPins()
{
    if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        m_logStream->println(F("-> RotarySwitch::taskReadPins is online."));
        xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    m_lastStatePinSwitch = digitalRead(m_pinSwitch);
    m_lastStatePinClk = digitalRead(m_pinClk);
    m_lastStatePinDt = digitalRead(m_pinDt);

    vTaskDelay(pdMS_TO_TICKS(1));

    for (;;) {
        m_statePinClk = digitalRead(m_pinClk);
        m_statePinDt = digitalRead(m_pinDt);
        m_statePinSwitch = digitalRead(m_pinSwitch);

        if (m_statePinClk != m_lastStatePinClk || m_statePinDt != m_lastStatePinDt) {
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->print(F("taskReadPins read data pins:"));
                m_logStream->print(m_lastStatePinClk);
                m_logStream->print(F(" "));
                m_logStream->print(m_statePinClk);
                m_logStream->print(F(" : "));
                m_logStream->print(m_lastStatePinDt);
                m_logStream->print(F(" "));
                m_logStream->print(m_statePinDt);
                m_logStream->print(F(" : "));
                m_logStream->print(m_lastStatePinSwitch);
                m_logStream->print(F(" "));
                m_logStream->println(m_statePinSwitch);

                xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
            }
        }

        // uint8_t state = (m_lastStatePinClk << 3) | (m_statePinClk << 2) | (m_lastStatePinDt << 1) | m_statePinDt;

        // int8_t mouvement = ENCODER_STATES[state];

        // if (mouvement > 0) {
        //     if (m_rotaryCallbackChanged != nullptr)
        //         m_rotaryCallbackChanged(m_id, true);
        //     if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        //         m_logStream->println(F("taskReadPins rotary turn clockwize"));

        //         xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
        //     }
        // } else if (mouvement < 0) {
        //     if (m_rotaryCallbackChanged != nullptr)
        //         m_rotaryCallbackChanged(m_id, false);
        //     if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        //         m_logStream->println(F("taskReadPins rotary turn counterclockwize"));

        //         xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
        //     }
        // }

        if ((m_lastStatePinClk == HIGH) &&(m_statePinClk == LOW) &&  (m_lastStatePinDt == HIGH) && (m_statePinDt == HIGH)) {
            // if (m_rotaryCallbackChanged != nullptr)
            m_rotaryCallbackChanged(m_id, true);
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->println(F("taskReadPins rotary turn clockwize"));

                xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
            }
        }

        if ((m_lastStatePinClk == HIGH) &&(m_statePinClk == HIGH) &&  (m_lastStatePinDt == HIGH) && (m_statePinDt == LOW)) {
            // if (m_rotaryCallbackChanged != nullptr)
            m_rotaryCallbackChanged(m_id, false);
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->println(F("taskReadPins rotary turn counterclockwize"));

                xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
            }
        }

        if (m_lastStatePinSwitch == LOW && m_statePinSwitch == HIGH) {
            // if (m_pushButtonCallbackClicked != nullptr)
            m_pushButtonCallbackClicked(m_id);
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->println(F("taskReadPins rotary turn click"));

                xSemaphoreGive(m_xLogStreamSemaphore); // On rend la clé immédiatement
            }
        }

        m_lastStatePinClk = m_statePinClk;
        m_lastStatePinDt = m_statePinDt;
        m_lastStatePinSwitch = m_statePinSwitch;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// void RotarySwitch::taskReadPins()
// {
//     if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
//         m_logStream->println(F("-> taskReadPins is online."));
//         xSemaphoreGive(m_xLogStreamSemaphore);
//     }

//     vTaskDelay(pdMS_TO_TICKS(200));

//     m_lastStatePinClk = digitalRead(m_pinClk);
//     m_lastStatePinSwitch = digitalRead(m_pinSwitch);

//     // Variables pour stocker le moment du dernier événement valide
//     TickType_t lastRotaryTime = xTaskGetTickCount();
//     TickType_t lastClickTime = xTaskGetTickCount();

//     // Définition des temps morts (à ajuster selon la qualité de ton switch)
//     const TickType_t rotaryDebounceDelay = pdMS_TO_TICKS(10); // 10 ms pour la rotation
//     const TickType_t clickDebounceDelay = pdMS_TO_TICKS(50);  // 50 ms pour le gros clic mécanique

//     for (;;) {
//         TickType_t currentTime = xTaskGetTickCount();

//         m_statePinClk = digitalRead(m_pinClk);
//         m_statePinSwitch = digitalRead(m_pinSwitch);

//         // 1. DÉTECTION DU MOUVEMENT
//         if (m_lastStatePinClk == LOW && m_statePinClk == HIGH) {
//             // Est-ce qu'on a dépassé le temps mort depuis le dernier mouvement ?
//             if ((currentTime - lastRotaryTime) > rotaryDebounceDelay) {
//                 m_statePinDt = digitalRead(m_pinDt);

//                 if (m_statePinDt == LOW) {
//                     if (m_rotaryCallbackChanged != nullptr) m_rotaryCallbackChanged(m_id, true);
//                 } else {
//                     if (m_rotaryCallbackChanged != nullptr) m_rotaryCallbackChanged(m_id, false);
//                 }

//                 lastRotaryTime = currentTime; // On réinitialise le chronomètre
//             }
//         }

//         // 2. DÉTECTION DU BOUTON POUSSOIR
//         if (m_lastStatePinSwitch == LOW && m_statePinSwitch == HIGH) {
//             // Est-ce qu'on a dépassé le temps mort pour le clic ?
//             if ((currentTime - lastClickTime) > clickDebounceDelay) {
//                 if (m_pushButtonCallbackClicked != nullptr) {
//                     m_pushButtonCallbackClicked(m_id);
//                 }

//                 lastClickTime = currentTime; // On réinitialise le chronomètre
//             }
//         }

//         m_lastStatePinClk = m_statePinClk;
//         m_lastStatePinSwitch = m_statePinSwitch;

//         // On laisse respirer le CPU (1 tick = souvent 1ms ou 10ms selon ta config)
//         vTaskDelay(1);
//     }
// }

void RotarySwitch::setCallbackClicked(void (*pushButtonCallbackClicked)(uint8_t id)) { m_pushButtonCallbackClicked = pushButtonCallbackClicked; }

void RotarySwitch::setRotaryCallbackChanged(void (*rotaryCallbackChanged)(uint8_t id, bool clockWize)) { m_rotaryCallbackChanged = rotaryCallbackChanged; }
