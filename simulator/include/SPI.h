#pragma once

#include <stddef.h>
#include <stdint.h>

#define MSBFIRST 1
#define LSBFIRST 0
#ifndef SPI_HAS_TRANSACTION
#define SPI_HAS_TRANSACTION 1
#endif
typedef uint8_t BitOrder;
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

class SPISettings {
public:
    SPISettings();
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode);
    uint32_t clock;
    uint8_t bitOrder;
    uint8_t dataMode;
};

class SPIClass {
public:
    // SCK, MISO, MOSI, SS. -1 keeps the XIAO defaults (SCK 7, MISO 8, MOSI 9).
    // transfer() does not toggle CS. The LCD chip select is GPIO2.
    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
    void end();
    void beginTransaction(SPISettings settings);
    void endTransaction();
    void setFrequency(uint32_t freq);
    void setBitOrder(uint8_t order);
    void setDataMode(uint8_t mode);
    uint8_t transfer(uint8_t data);
    void transfer(void* buf, size_t count);
    void transferBytes(const uint8_t* data, uint8_t* out, uint32_t size);
    void write(uint8_t data);
    void writeBytes(const uint8_t* data, uint32_t size);
};

extern SPIClass SPI;
