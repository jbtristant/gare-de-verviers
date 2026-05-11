/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"
#include "led.h"
#include "rotaryswitch.h"

#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Constantes
#define ROTARY_SWITCH_SW_PIN  2       // La pin D2 de l'Arduino recevra la ligne SW du module KY-040
#define ROTARY_SWITCH_CLK_PIN 3       // La pin D3 de l'Arduino recevra la ligne CLK du module KY-040
#define ROTARY_SWITCH_DT_PIN  4       // La pin D4 de l'Arduino recevra la ligne DT du module KY-040

#define PIN_WS2812B 5  // Arduino pin that connects to WS2812B
#define NUM_PIXELS 12  // The number of LEDs (pixels) on WS2812B

#define DELAY_INTERVAL 250 // 250ms pause between each pixel

Adafruit_NeoPixel WS2812B(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

Led led(LED_BUILTIN);
RotarySwitch rotarySwitch(ROTARY_SWITCH_SW_PIN, ROTARY_SWITCH_CLK_PIN, ROTARY_SWITCH_DT_PIN, 0, true);


int compteur = 0;                   // Cette variable nous permettra de compter combien de crans ont été parcourus, sur l'encodeur
                                     // (sachant que nous compterons dans le sens horaire, et décompterons dans le sens antihoraire)


void onRotarySwitchClicked(uint8_t id);
void onRotarySwitchChanged(uint8_t id, bool clockWize);

// ========================
// Initialisation programme
// ========================
void setup() {

    // Initialisation de la liaison série (arduino nano <-> PC)
    Serial.begin(115200);

    WS2812B.begin();
    WS2812B.setBrightness(32);
    WS2812B.clear();
    WS2812B.show();

    led.begin();

    rotarySwitch.setCallbackClicked(onRotarySwitchClicked);
    rotarySwitch.setRotaryCallbackChanged(onRotarySwitchChanged);

    rotarySwitch.begin();

    // Affichage de la valeur initiale du compteur, sur le moniteur série
    Serial.print(F("Valeur initiale du compteur = "));
    Serial.println(compteur);

}

// =================
// Boucle principale
// =================
void loop() {
    rotarySwitch.process();  
}


void onRotarySwitchClicked(uint8_t id)
{
    led.toggle();
}

void onRotarySwitchChanged(uint8_t id, bool clockWize)
{
    if (clockWize) {
        ++compteur;
        Serial.print(F("Sens = horaire | Valeur du compteur = "));
    } else {
        --compteur;
        Serial.print(F("Sens = antihoraire | Valeur du compteur = "));
    }
    Serial.println(compteur);

    WS2812B.clear();
    for (int pixel = 0; pixel < min(NUM_PIXELS, compteur); ++pixel) {           // for each pixel
        WS2812B.setPixelColor(pixel, WS2812B.Color(22, 36, 16));  // it only takes effect if pixels.show() is called
    }
    WS2812B.show();   
}

