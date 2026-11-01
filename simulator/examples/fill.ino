// Load this from the simulator window to see the glass turn red.
// The panel is 240x280 and shows ST7789 memory rows 20 through 299.

#include <SPI.h>

const int PIN_BL = 1;
const int PIN_CS = 2;
const int PIN_DC = 3;
const int PIN_RST = 4;

void lcd_cmd(uint8_t value) {
    digitalWrite(PIN_DC, LOW);
    digitalWrite(PIN_CS, LOW);
    SPI.transfer(value);
    digitalWrite(PIN_CS, HIGH);
}

void lcd_data(uint8_t value) {
    digitalWrite(PIN_DC, HIGH);
    digitalWrite(PIN_CS, LOW);
    SPI.transfer(value);
    digitalWrite(PIN_CS, HIGH);
}

void setup() {
    pinMode(PIN_BL, OUTPUT);
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_RST, HIGH);
    digitalWrite(PIN_BL, HIGH);

    SPI.begin();
    delay(20);
    lcd_cmd(0x11);
    delay(120);
    lcd_cmd(0x3A);
    lcd_data(0x55);
    lcd_cmd(0x29);
    delay(20);

    lcd_cmd(0x2A);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data(239);
    lcd_cmd(0x2B);
    lcd_data(0x00);
    lcd_data(20);
    lcd_data(0x01);
    lcd_data(0x2B);
    lcd_cmd(0x2C);

    digitalWrite(PIN_DC, HIGH);
    digitalWrite(PIN_CS, LOW);
    for (int i = 0; i < 240 * 280; i++) {
        SPI.transfer(0xF8);
        SPI.transfer(0x00);
    }
    digitalWrite(PIN_CS, HIGH);
    Serial.println("glass filled");
}

void loop() {
}
