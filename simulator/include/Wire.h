#pragma once

#include <stddef.h>
#include <stdint.h>

class TwoWire {
public:
    // SDA, SCL. -1 keeps the XIAO defaults (SDA 5, SCL 6), which are the touch pins.
    bool begin(int sda = -1, int scl = -1, uint32_t frequency = 0);
    void end();
    void setClock(uint32_t frequency);
    void beginTransmission(uint8_t address);
    uint8_t endTransmission(uint8_t sendStop = 1);
    size_t write(uint8_t data);
    size_t write(const uint8_t* data, size_t len);
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop = 1);
    int available();
    int read();
    int peek();
};

extern TwoWire Wire;
