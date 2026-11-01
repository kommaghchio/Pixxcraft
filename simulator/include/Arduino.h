#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pins_arduino.h"
#include "Print.h"

// Adafruit (and most Arduino libraries) choose their headers from this.
#ifndef ARDUINO
#define ARDUINO 10819
#endif

#define HIGH 0x1
#define LOW 0x0

#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define INPUT_PULLDOWN 0x09

#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03

#define DEC 10
#define HEX 16
#define BIN 2
#define OCT 8

#define PI 3.14159265358979323846
#define DEG_TO_RAD 0.01745329251994329577
#define RAD_TO_DEG 57.2957795130823208768
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)

#define F(s) (s)
#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#define IRAM_ATTR
#define digitalPinToInterrupt(pin) (pin)

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet((value), (bit)) : bitClear((value), (bit)))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef constrain
#define constrain(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#endif

typedef uint8_t byte;
typedef bool boolean;
typedef uint16_t word;

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void analogWrite(uint8_t pin, int duty);

bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
bool ledcWrite(uint8_t pin, uint32_t duty);
bool ledcDetach(uint8_t pin);

void attachInterrupt(uint8_t pin, void (*fn)(), int mode);
void detachInterrupt(uint8_t pin);

void delay(uint32_t ms);
void delayMicroseconds(unsigned int us);
uint32_t millis();
uint32_t micros();
void yield();

long random(long max);
long random(long min_value, long max_value);
void randomSeed(unsigned long seed);
long map(long x, long in_min, long in_max, long out_min, long out_max);

class SerialClass : public Stream {
public:
    void begin(unsigned long baud);
    explicit operator bool() const { return true; }
    size_t write(uint8_t c);
    size_t write(const uint8_t* buf, size_t len);
    void flush();
    int available();
    int read();
    int peek() override { return -1; }
    void printf(const char* fmt, ...);

    void print(const char* s);
    void print(char c);
    void print(int v, int base = DEC);
    void print(unsigned v, int base = DEC);
    void print(long v, int base = DEC);
    void print(unsigned long v, int base = DEC);
    void print(long long v, int base = DEC);
    void print(unsigned long long v, int base = DEC);
    void print(double v, int digits = 2);

    template <typename T>
    void println(const T& v) {
        print(v);
        write('\n');
    }
    template <typename T, typename U>
    void println(const T& v, const U& extra) {
        print(v, extra);
        write('\n');
    }
    void println() { write('\n'); }
};

extern SerialClass Serial;

#include "WString.h"
