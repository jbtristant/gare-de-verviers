/*
        Created by Jean-Benoit Tristant, May 11, 2026
*/

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <U8g2lib.h>
#include <queue.h>
#include <semphr.h>

#include "commandstationclient.h"
#include "locomotive.h"
#include "mainmenu.h"
#include "rotaryswitch.h"
#include "turnout.h"

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Constantes
#define ROTARY_SWITCH_SW_PIN 2
#define ROTARY_SWITCH_CLK_PIN 3
#define ROTARY_SWITCH_DT_PIN 5

#define PIN_WS2812B 6
#define NUM_PIXELS 12

SemaphoreHandle_t xSerialSemaphore = nullptr;
SemaphoreHandle_t xSerial2Semaphore = nullptr;
CommandStationClient commandStationClient;
RotarySwitch rotarySwitch(ROTARY_SWITCH_SW_PIN, ROTARY_SWITCH_CLK_PIN, ROTARY_SWITCH_DT_PIN, 0);
Adafruit_NeoPixel WS2812B(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
MainMenu mainMenu;

void onRotarySwitchClicked(uint8_t id);
void onRotarySwitchChanged(uint8_t id, bool clockWize);
void on_track_changed();
void on_locoSpeed_changed(uint16_t address, int8_t speed, Direction direction);
void on_turnoutState_changed(uint16_t id, TurnoutState state);

void updateThrottleDisplay(int8_t throttleValue, Direction direction);

// ========================
// Initialisation programme
// ========================
void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200);

    xSerialSemaphore = xSemaphoreCreateMutex();
    xSerial2Semaphore = xSemaphoreCreateMutex();

    if (xSerialSemaphore == nullptr || xSerial2Semaphore == nullptr) {
        Serial.println(F("ERREUR FATALE : Impossible d'allouer les sémaphore."));
        return;
    } else {
        Serial.println(F("Sémaphore alloué."));
    }

    commandStationClient.initialize(&Serial2, xSerial2Semaphore, &Serial, xSerialSemaphore);

    WS2812B.begin();
    WS2812B.setBrightness(32);
    WS2812B.clear();
    WS2812B.show();

    u8g2.begin();
    u8g2.enableUTF8Print();

    rotarySwitch.setCallbackClicked(onRotarySwitchClicked);
    rotarySwitch.setRotaryCallbackChanged(onRotarySwitchChanged);

    rotarySwitch.initialize(&Serial, xSerialSemaphore);

    mainMenu.initialize(&commandStationClient, &u8g2, &Serial, xSerialSemaphore);

    commandStationClient.setLocoSpeedChangedCallback(on_locoSpeed_changed);

    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.println(F("Main::Setup vTaskStartScheduler"));
        xSemaphoreGive(xSerialSemaphore);
    }
    vTaskStartScheduler();
}

void loop() {}

void onRotarySwitchClicked(uint8_t id)
{
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.println(F("Rotary switch clicked"));
        xSemaphoreGive(xSerialSemaphore);
    }

    mainMenu.buttonPress();
}

void onRotarySwitchChanged(uint8_t id, bool clockWize)
{
    if (clockWize) {
        if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            Serial.println(F("Rotary switch turn clockwize"));
            xSemaphoreGive(xSerialSemaphore);
        }
        mainMenu.menuDown();
    } else {
        if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            Serial.println(F("Rotary switch turn counter clockwize"));
            xSemaphoreGive(xSerialSemaphore);
        }
        mainMenu.menuUp();
    }
}

void on_track_changed()
{
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.println(F("Track changed"));
        xSemaphoreGive(xSerialSemaphore);
    }
    mainMenu.onTrackChanged();
}

void on_locoSpeed_changed(uint16_t address, int8_t speed, Direction direction)
{
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.print(F("Speed: "));
        Serial.println(speed);
        xSemaphoreGive(xSerialSemaphore);
    }

    updateThrottleDisplay(speed, direction);
    mainMenu.onLocomotiveChanged();
}

void on_turnoutState_changed(uint16_t id, TurnoutState state)
{
    if (xSemaphoreTake(xSerialSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        Serial.print(F("Turnout "));
        Serial.print(id);
        Serial.print(F(" changed to "));
        Serial.println((uint8_t)state);
        xSemaphoreGive(xSerialSemaphore);
    }
    mainMenu.onTurnoutStateChanged(id, state);
}

uint8_t gammaCorrect(uint8_t val)
{
    // Formule simple (x^2 / 255) qui redonne du contraste
    return (uint16_t)val * val / 255;
}

void updateThrottleDisplay(int8_t throttleValue, Direction direction)
{
    // 1. Core Logic & Direction
    uint8_t absValue = (throttleValue > 127) ? 127 : throttleValue;
    bool isLimitReached = (absValue >= 127);

    // 2. Alert / Breathing Logic
    float pulse = isLimitReached ? ((sin(millis() / 100.0) + 1.0) / 2.0) : 1.0;

    // 3. Fluid Position Calculation (your 0.2 offset)
    float precisePosition = 0;
    if (absValue > 0) {
        precisePosition = 0.2 + ((float)(absValue - 1) * 11.8 / 126.0);
    }

    // 4. LED Rendering Loop
    for (int i = 0; i < NUM_PIXELS; i++) {
        // Reverse mapping logic
        int ledIndex = (direction == Direction::Reverse) ? (NUM_PIXELS - 1 - i) : i;

        // Calculate filling intensity (0.0 to 1.0)
        float diff = precisePosition - i;
        float fillIntensity = 0.0;
        if (diff >= 1.0)
            fillIntensity = 1.0;
        else if (diff > 0.0)
            fillIntensity = diff;

        // Color Gradient calculation (Ratio 0.0 to 1.0)
        float colorRatio = (float)i / (NUM_PIXELS - 1);
        uint8_t r_raw = 0, g_raw = 0, b_raw = 0;

        if (colorRatio < 0.5) {
            float segment = colorRatio * 2.0;
            r_raw = (uint8_t)(255 * segment);
            g_raw = 255;
        } else {
            float segment = (colorRatio - 0.5) * 2.0;
            r_raw = 255;
            g_raw = (uint8_t)(255 * (1.0 - segment));
        }

        // Apply intensity and Gamma Correction
        uint8_t r = gammaCorrect((uint8_t)(r_raw * fillIntensity * pulse));
        uint8_t g = gammaCorrect((uint8_t)(g_raw * fillIntensity * pulse));
        uint8_t b = gammaCorrect((uint8_t)(b_raw * fillIntensity * pulse));

        WS2812B.setPixelColor(ledIndex, WS2812B.Color(r, g, b));
    }
    WS2812B.show();
}