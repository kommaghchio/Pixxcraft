#pragma once

// XIAO ESP32-S3 pins to the Waveshare 1.69" module, matching the module
// silkscreen (LCD_DIN on GPIO9, LCD_CLK on GPIO7, TP_RST on GPIO44, TP_IRQ on GPIO43).

constexpr int kPinBl = 1;
constexpr int kPinCs = 2;
constexpr int kPinDc = 3;
constexpr int kPinRst = 4;
constexpr int kPinTpSda = 5;
constexpr int kPinTpScl = 6;
constexpr int kPinTpInt = 43;
constexpr int kPinTpRst = 44;
constexpr int kPinSck = 7;
constexpr int kPinMiso = 8;
constexpr int kPinMosi = 9;
constexpr int kPinLed = 21;

constexpr int kGramW = 240;
constexpr int kGramH = 320;
constexpr int kGlassW = 240;
constexpr int kGlassH = 280;
// The ST7789 RAM is 320 rows. This panel's glass is bonded to rows 20..299.
constexpr int kGlassY = 20;
