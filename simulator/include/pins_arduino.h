#pragma once

#include <stdint.h>

// GPIO numbers for the Seeed XIAO ESP32-S3 Arduino variant.
// D6/D7 are the touch interrupt and reset nets on this board, so USB
// Serial is the log port. Do not treat TX/RX as a free UART.

static const uint8_t LED_BUILTIN = 21;
#define LED_BUILTIN LED_BUILTIN
#define BUILTIN_LED LED_BUILTIN

static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 5;
static const uint8_t SCL = 6;

static const uint8_t SS = 44;
static const uint8_t MOSI = 9;
static const uint8_t MISO = 8;
static const uint8_t SCK = 7;

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A8 = 7;
static const uint8_t A9 = 8;
static const uint8_t A10 = 9;

static const uint8_t D0 = 1;
static const uint8_t D1 = 2;
static const uint8_t D2 = 3;
static const uint8_t D3 = 4;
static const uint8_t D4 = 5;
static const uint8_t D5 = 6;
static const uint8_t D6 = 43;
static const uint8_t D7 = 44;
static const uint8_t D8 = 7;
static const uint8_t D9 = 8;
static const uint8_t D10 = 9;
