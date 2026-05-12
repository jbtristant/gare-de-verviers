/*
	Created by Jean-Benoit Tristant, May 11, 2026
*/

#include "Arduino.h"
#include "led.h"
#include "pushbutton.h"
#include "rotaryswitch.h"

#include <SPI.h>
#include <Ethernet.h>
//#include <DCCEXProtocol.h>

#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Constantes
#define ROTARY_SWITCH_SW_PIN  2       // La pin D2 de l'Arduino recevra la ligne SW du module KY-040
#define ROTARY_SWITCH_CLK_PIN 3       // La pin D3 de l'Arduino recevra la ligne CLK du module KY-040
#define ROTARY_SWITCH_DT_PIN  5       // La pin D4 de l'Arduino recevra la ligne DT du module KY-040

#define PIN_WS2812B 6  // Arduino pin that connects to WS2812B
#define NUM_PIXELS 12  // The number of LEDs (pixels) on WS2812B

#define DELAY_INTERVAL 250 // 250ms pause between each pixel

byte mac[] = {
  0x42, 0x34, 0x34,
  0x00, 0x00, 0x02
};

EthernetClient client;
//DCCEXProtocol dccexProtocol;

Adafruit_NeoPixel WS2812B(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

PushButton pushButton(7, 0);
Led led(LED_BUILTIN);
RotarySwitch rotarySwitch(ROTARY_SWITCH_SW_PIN, ROTARY_SWITCH_CLK_PIN, ROTARY_SWITCH_DT_PIN, 0, true);


bool alerteVisible = true;
unsigned long lastBreathing = 0;
const int BREATHING = 25; // Vitesse du clignotement en ms
int throttle = 0; 

IPAddress commandStationIP(192,168,11,53);
const uint16_t commandStationPort = 2560;

/// TCP Reading
char inData[32];
char inChar = -1;
byte arraySize = 0;

void connectToCommandStation();
void onRotarySwitchClicked(uint8_t id);
void onRotarySwitchChanged(uint8_t id, bool clockWize);
void updateThrottleDisplay(int throttleValue);
void on_pushButton_pushed(int id);
void processMessage();
void processCmd(char data[], byte size);

// ========================
// Initialisation programme
// ========================
void setup() {

    // Initialisation de la liaison série (arduino nano <-> PC)
    Serial.begin(115200);
    Serial.println(F("Start ethernet"));

    if (Ethernet.begin(mac) == 0) {
        Serial.println(F("Failed to configure Ethernet using DHCP"));
        // no point in carrying on, so do nothing forevermore:
        while(true);
    }

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
        }
    }
    while (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
        delay(500);
    }

    // give the Ethernet shield a second to initialize:
    delay(1000);

    // print your local IP address:
    Serial.print(F("My IP address: "));
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        // print the value of each byte of the IP address:
        Serial.print(Ethernet.localIP()[thisByte], DEC);
        Serial.print(".");
        }
    Serial.println();

    connectToCommandStation();

    WS2812B.begin();
    WS2812B.setBrightness(32);
    WS2812B.clear();
    WS2812B.show();

    pushButton.setCallbackClicked(on_pushButton_pushed);
    pushButton.begin();

    led.begin();

    rotarySwitch.setCallbackClicked(onRotarySwitchClicked);
    rotarySwitch.setRotaryCallbackChanged(onRotarySwitchChanged);

    rotarySwitch.begin();

    // Affichage de la valeur initiale du compteur, sur le moniteur série
    Serial.print(F("Valeur initiale du compteur = "));
    Serial.println(throttle);

}

// =================
// Boucle principale
// =================
void loop() {
    rotarySwitch.process(); 
    pushButton.process();
    
    // Si on est aux limites, on force le rafraîchissement pour le clignotement
    if ((abs(throttle) >= 127) && (millis() - lastBreathing >= BREATHING)) {
        updateThrottleDisplay(throttle);
        lastBreathing = millis();
    }

    if (!client.connected()) {
        Serial.println(F("\nClient déconnecté, tentative de reconexion..."));
        client.stop();
        connectToCommandStation();
    }

    processMessage();
}

void connectToCommandStation()
{
    while (!client.connect(commandStationIP, commandStationPort)) {
        Serial.println(F("Connexion to CommandStation..."));
        delay(2000);
    }

    Serial.println(F("Connecté à CommandStation"));
    client.print(F("<s>"));

    //dcc.setLogStream(&Serial);
    //dcc.connect(&client);
}

void on_pushButton_pushed(int id)
{
    Serial.print("pushButton ");
    Serial.print(id);
    Serial.println(" pressed");

    switch (id) {
        case 0:
        client.print(F("<1>"));
        break;
    }
}

void onRotarySwitchClicked(uint8_t id)
{
        Serial.println("<t 64 0 1>");
        client.print("<t 64 0 1>");
}


void onRotarySwitchChanged(uint8_t id, bool clockWize)
{
    int pas = 4; 
    if (clockWize) {
        throttle = min(throttle + pas, 127);
        //Serial.print(F("Sens = horaire | Valeur du compteur = "));
    } else {
        throttle = max(throttle - pas, -127);
        //Serial.print(F("Sens = antihoraire | Valeur du compteur = "));
    }
    if (abs(throttle) <= 1) throttle = 0;
    //Serial.println(throttle);

    // --- ENVOI À LA CENTRALE ---
    // Exemple pour la loco adresse 3 (adresse par défaut souvent)
    // Syntaxe : <t 1 3 vitesse direction> 
    // vitesse : 0-126, direction : 1 (avant) ou 0 (arrière)
    
    int dccSpeed = abs(throttle);
    int dccDir = (throttle >= 0) ? 1 : 0;
    
    if (client.connected()) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "<t 64 %d %d>", dccSpeed, dccDir);
        Serial.println(buffer);
        client.print(buffer);
    }
    // ---------------------------

    //updateThrottleDisplay(throttle);   
}


uint8_t gammaCorrect(uint8_t val) {
    // Formule simple (x^2 / 255) qui redonne du contraste
    return (uint16_t)val * val / 255; 
}

// Translation of your Premium function
void updateThrottleDisplay(int throttleValue) {
    // 1. Core Logic & Direction
    bool isReverse = (throttleValue < 0);
    uint8_t absValue = min(abs(throttleValue), 127);
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
        int ledIndex = isReverse ? (NUM_PIXELS - 1 - i) : i;

        // Calculate filling intensity (0.0 to 1.0)
        float diff = precisePosition - i;
        float fillIntensity = 0.0;
        if (diff >= 1.0) fillIntensity = 1.0;
        else if (diff > 0.0) fillIntensity = diff;

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

void processMessage()
{
    while (client.available() > 0) {
        if (arraySize < 31) { // One less than the size of the array
            inChar = client.read(); // Read a character
            if (inChar == '<') {
                arraySize = 0;
                return;
            }
            if (inChar == '>') {
                inData[arraySize] = '\0';
                processCmd(inData, arraySize);
            }
            inData[arraySize] = inChar; // Store it
            arraySize++; // Increment where to write next
            inData[arraySize] = '\0'; // Null terminate the string
        } else {
            arraySize = 0;
        }
    }
}

void processCmd(char data[], byte size)
{
    Serial.println(inData);
    // On vérifie si le premier caractère est un 'l' (minuscule)
    if (data[0] == 'l') {
        int cabid, slot, speedbyte;
        long functionMap;

        // sscanf cherche le format : l [espace] entier [espace] entier...
        // On commence à data+1 pour ignorer le 'l'
        int matched = sscanf(data + 1, "%d %d %d %ld", &cabid, &slot, &speedbyte, &functionMap);

        if (matched >= 3) {
            int realThrottle = 0;

            // Conversion du SpeedByte en ton système (-127 à 127)
            if (speedbyte >= 130) {
                realThrottle = speedbyte - 129; // Marche avant
            } else if (speedbyte >= 2 && speedbyte <= 127) {
                realThrottle = -(speedbyte - 1); // Marche arrière
            } else {
                realThrottle = 0; // Arrêt (0 ou 128)
            }

            // Filtrage : On ne met à jour que si c'est notre loco (64)
            if (cabid == 64) {
                throttle = realThrottle;
                updateThrottleDisplay(throttle);
                
                Serial.print(F("Update Reçu - Loco 64 | Vitesse: "));
                Serial.println(throttle);
            }
        }
    } else if (data[0] == 'X') {
        Serial.println(F("Commande refusée par la centrale."));
    } else {
        // Affiche les autres messages (ID, Power status, etc.)
        Serial.print(F("Autre message : "));
        Serial.println(data);
    }
}