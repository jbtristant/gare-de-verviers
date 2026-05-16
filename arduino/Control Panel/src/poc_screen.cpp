#include "Arduino.h"
#include <U8g2lib.h>
#include "images.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int throttle = 0; 
unsigned long lastOledDisplay = 0;

void updateOLED();

void setup() {
  // put your setup code here, to run once:
    u8g2.begin();
    u8g2.enableUTF8Print();
    throttle = 142;

}

void loop() {
  // put your main code here, to run repeatedly:
    if (lastOledDisplay + 100 < millis()) {
        updateOLED();
        lastOledDisplay = millis();
    }
}

void updateOLED() {
    u8g2.clearBuffer();
    
    // Cadre décoratif style ancien
    //u8g2.drawFrame(0, 0, 128, 64);
    //u8g2.drawLine(0, 15, 128, 15);

    // Titre avec accents
    //u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);
    u8g2.drawUTF8(0, 12, "Gare de Verviers-Central");

    // Valeur du Throttle
    //u8g2.setFont(u8g2_font_logisoso32_tn); 
    //u8g2.setFont(u8g2_font_logisoso32_tf); // Grande police pour la vitesse
    char buf[8];
    itoa(throttle, buf, 10);
    u8g2.drawStr(0, 50, buf);

    u8g2.drawXBMP(0, 14, 24, 24, epd_bitmap_icons8_snail_24);
    u8g2.drawXBMP(26, 14, 24, 24, epd_bitmap_icons8_turtle_24);
    u8g2.drawXBMP(52, 14, 24, 24, epd_bitmap_icons8_rabbit_24);
    u8g2.drawXBMP(78, 14, 24, 24, epd_bitmap_icons8_hummingbird_24);
    u8g2.drawXBMP(104, 14, 24, 24, epd_bitmap_icons8_cat_24);

    u8g2.sendBuffer(); // Envoi d'un coup, très fluide sur Mega
}