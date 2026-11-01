#include <cstdio>
#include <cstring>

#include "board.h"
#include "Arduino.h"
#include "SPI.h"
#include "Wire.h"
#include "host.h"
#include "log.h"
#include "sim_api.h"

namespace {

int g_fails = 0;

void expect(bool cond, const char* msg) {
    if (cond) {
        std::printf("  ok   %s\n", msg);
    } else {
        std::printf("  FAIL %s\n", msg);
        g_fails++;
    }
}

void expect_rgb(int x, int y, uint32_t rgb, const char* msg) {
    uint32_t got = sim_glass_rgb(x, y);
    if (got == rgb) {
        std::printf("  ok   %s\n", msg);
    } else {
        std::printf("  FAIL %s (got #%06X want #%06X)\n", msg, got, rgb);
        g_fails++;
    }
}

void cmd(uint8_t value) {
    digitalWrite(D2, LOW);
    digitalWrite(D1, LOW);
    SPI.transfer(value);
    digitalWrite(D1, HIGH);
}

void data_byte(uint8_t value) {
    digitalWrite(D2, HIGH);
    digitalWrite(D1, LOW);
    SPI.transfer(value);
    digitalWrite(D1, HIGH);
}

void set_window(int x0, int y0, int x1, int y1) {
    cmd(0x2A);
    data_byte((uint8_t)(x0 >> 8));
    data_byte((uint8_t)x0);
    data_byte((uint8_t)(x1 >> 8));
    data_byte((uint8_t)x1);
    cmd(0x2B);
    data_byte((uint8_t)(y0 >> 8));
    data_byte((uint8_t)y0);
    data_byte((uint8_t)(y1 >> 8));
    data_byte((uint8_t)y1);
    cmd(0x2C);
}

void pixel(uint16_t color) {
    digitalWrite(D2, HIGH);
    digitalWrite(D1, LOW);
    SPI.transfer((uint8_t)(color >> 8));
    SPI.transfer((uint8_t)color);
    digitalWrite(D1, HIGH);
}

void prep_pins() {
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);
    digitalWrite(D1, HIGH);
    digitalWrite(D0, HIGH);
    digitalWrite(D3, HIGH);
}

void boot() {
    prep_pins();
    digitalWrite(D3, LOW);
    delay(2);
    digitalWrite(D3, HIGH);
    delay(2);
    SPI.begin();
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    cmd(0x11);
    cmd(0x29);
    cmd(0x3A);
    data_byte(0x55);
    cmd(0x36);
    data_byte(0x00);
}

volatile int g_falls = 0;

void on_falling() {
    g_falls++;
}

}  // namespace

int run_self_test(const char* dump_path) {
    std::printf("\nself-test\n");

    prep_pins();
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    cmd(0x11);
    cmd(0x29);
    set_window(0, 20, 0, 20);
    pixel(0xF800);
    expect(sim_warning_count() > 0, "RAM write before COLMOD is reported");

    int before_pins = sim_warning_count();
    SPI.end();
    SPI.begin(12, -1, 11, -1);
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
    SPI.transfer(0x29);
    digitalWrite(D1, HIGH);
    expect(sim_warning_count() > before_pins, "SPI on the wrong pins is reported");

    SPI.end();
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(D2, LOW);
    digitalWrite(D1, LOW);
    SPI.transfer(0x00);
    digitalWrite(D1, HIGH);
    int before_cs = sim_warning_count();
    SPI.transfer(0x11);
    expect(sim_warning_count() > before_cs, "SPI byte with CS high is reported");

    boot();

    cmd(0x36);
    data_byte(0x40);
    set_window(10, 20, 11, 20);
    pixel(0xF800);
    pixel(0x001F);
    expect_rgb(11, 0, 0x00FF0000, "MADCTL MX puts the first pixel on the right");
    expect_rgb(10, 0, 0x000000FF, "MADCTL MX puts the second pixel to its left");
    cmd(0x36);
    data_byte(0x00);

    cmd(0x36);
    data_byte(0x08);
    set_window(4, 20, 4, 20);
    pixel(0xF800);
    expect_rgb(4, 0, 0x000000FF, "MADCTL BGR swaps a red write into blue");
    cmd(0x36);
    data_byte(0x00);

    int before_off = sim_warning_count();
    set_window(0, 0, 0, 279);
    digitalWrite(D2, HIGH);
    digitalWrite(D1, LOW);
    for (int y = 0; y <= 279; y++) {
        uint16_t color = 0x0000;
        if (y == 0) {
            color = 0x001F;
        } else if (y == 20) {
            color = 0xFFFF;
        }
        SPI.transfer((uint8_t)(color >> 8));
        SPI.transfer((uint8_t)color);
    }
    digitalWrite(D1, HIGH);
    expect_rgb(0, 0, 0x00FFFFFF, "glass top shows GRAM row 20, not row 0");
    expect(sim_warning_count() > before_off, "writes above the glass are reported");

    Wire.begin();
    pinMode(D6, INPUT_PULLUP);
    g_falls = 0;
    attachInterrupt(digitalPinToInterrupt(D6), on_falling, FALLING);
    sim_force_touch(12, 34, true);
    expect(g_falls == 1, "touch INT falls when a finger lands");
    expect(digitalRead(D6) == LOW, "INT stays low while the finger is down");
    Wire.beginTransmission(0x15);
    Wire.write(0x02);
    expect(Wire.endTransmission(false) == 0, "CST816 at 0x15 acks");
    expect(Wire.requestFrom((uint8_t)0x15, (uint8_t)5) == 5, "five touch bytes");
    expect(Wire.read() == 1, "one finger");
    expect(Wire.read() == 0, "X high byte");
    expect(Wire.read() == 12, "X = 12");
    expect(Wire.read() == 0, "Y high byte");
    expect(Wire.read() == 34, "Y = 34");
    Wire.beginTransmission(0x20);
    expect(Wire.endTransmission() == 2, "unknown I2C address nacks");
    Wire.beginTransmission(0x2A);
    expect(Wire.endTransmission() == 2, "8-bit address 0x2A nacks");
    sim_force_touch(12, 34, false);
    expect(digitalRead(D6) == HIGH, "INT goes high on release");
    sim_release_touch_override();

    set_window(0, 20, 239, 299);
    uint8_t row[kGlassW * 2];
    for (int i = 0; i < kGlassW; i++) {
        row[i * 2] = 0xF8;
        row[i * 2 + 1] = 0x00;
    }
    digitalWrite(D2, HIGH);
    digitalWrite(D1, LOW);
    for (int y = 0; y < kGlassH; y++) {
        SPI.transferBytes(row, nullptr, sizeof row);
    }
    digitalWrite(D1, HIGH);
    set_window(0, 20, 0, 20);
    pixel(0xFFFF);
    set_window(239, 20, 239, 20);
    pixel(0x001F);
    set_window(0, 299, 0, 299);
    pixel(0x07E0);
    set_window(120, 160, 120, 160);
    pixel(0xFFE0);

    expect_rgb(0, 0, 0x00FFFFFF, "origin pixel is white");
    expect_rgb(1, 0, 0x00FF0000, "rest of the top row stays red");
    expect_rgb(239, 0, 0x000000FF, "top-right pixel is blue");
    expect_rgb(0, 279, 0x0000FF00, "bottom-left pixel is green");
    expect_rgb(120, 140, 0x00FFFF00, "center pixel is yellow");
    expect(sim_blank_reason()[0] == 0, "a lit, awake, enabled panel has no blank reason");

    ledcAttach(D0, 5000, 8);
    ledcWrite(D0, 0);
    expect_rgb(0, 0, 0, "backlight duty 0 hides the GRAM");
    expect(std::strstr(sim_blank_reason(), "backlight") != nullptr, "blank reason names the backlight");
    digitalWrite(D0, HIGH);
    expect_rgb(0, 0, 0x00FFFFFF, "driving GPIO1 high shows the pixel again");

    if (dump_path != nullptr && dump_path[0] != 0) {
        bool saved = sim_save_bmp(dump_path);
        expect(saved, "panel bitmap saved");
        char window_path[520];
        std::snprintf(window_path, sizeof window_path, "%s.window.bmp", dump_path);
        if (host_ready()) {
            expect(host_capture(window_path), "window bitmap saved");
        }
    }
    host_present();

    std::printf(g_fails == 0 ? "self-test passed\n" : "self-test failed: %d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
