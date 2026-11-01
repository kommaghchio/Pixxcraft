#include "cst816.h"

#include "board.h"
#include "gpio.h"
#include "log.h"

#include <cstdio>
#include <cstring>

namespace {

uint8_t g_regs[256];
uint8_t g_pointer = 0;
bool g_pointer_set = false;
bool g_override = false;
bool g_down = false;
int g_x = 0;
int g_y = 0;

void publish() {
    int x = g_x;
    int y = g_y;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x > kGlassW - 1) {
        x = kGlassW - 1;
    }
    if (y > kGlassH - 1) {
        y = kGlassH - 1;
    }
    g_x = x;
    g_y = y;
    g_regs[0x01] = 0x00;
    g_regs[0x02] = g_down ? 1 : 0;
    g_regs[0x03] = (uint8_t)((x >> 8) & 0x0F);
    g_regs[0x04] = (uint8_t)(x & 0xFF);
    g_regs[0x05] = (uint8_t)((y >> 8) & 0x0F);
    g_regs[0x06] = (uint8_t)(y & 0xFF);
    // Open-drain interrupt, active low while a finger is down.
    gpio_set_input_level(kPinTpInt, !g_down);
    gpio_fire_isrs();
}

}  // namespace

void cst816_init() {
    std::memset(g_regs, 0, sizeof g_regs);
    g_regs[0xA7] = 0xB5;
    g_pointer = 0;
    g_pointer_set = false;
    g_override = false;
    g_down = false;
    g_x = 0;
    g_y = 0;
    gpio_set_input_level(kPinTpInt, true);
}

bool cst816_in_reset() {
    return gpio_is_output(kPinTpRst) && !gpio_read(kPinTpRst);
}

bool cst816_touch_overridden() {
    return g_override;
}

void cst816_force_touch(int x, int y, bool down) {
    g_override = true;
    g_x = x;
    g_y = y;
    g_down = down;
    publish();
}

void cst816_clear_override() {
    g_override = false;
}

void cst816_apply_mouse(int x, int y, bool down) {
    if (g_override || cst816_in_reset()) {
        if (cst816_in_reset() && g_down) {
            g_down = false;
            publish();
        }
        return;
    }
    if (g_down == down && (!down || (g_x == x && g_y == y))) {
        return;
    }
    g_x = x;
    g_y = y;
    g_down = down;
    publish();
}

void cst816_set_pointer(uint8_t reg) {
    g_pointer = reg;
    g_pointer_set = true;
}

void cst816_write(uint8_t reg, const uint8_t* data, size_t len) {
    g_pointer = reg;
    g_pointer_set = true;
    for (size_t i = 0; i < len; i++) {
        g_regs[g_pointer] = data[i];
        g_pointer = (uint8_t)(g_pointer + 1);
    }
}

uint8_t cst816_read_next() {
    if (!g_pointer_set) {
        sim_warn_once(
            "i2c-noptr",
            "I2C read with no register selected. Write 0x02 (or 0x01) first; finger is at 0x02, X at 0x03, Y at 0x05.");
    }
    uint8_t value = g_regs[g_pointer];
    g_pointer = (uint8_t)(g_pointer + 1);
    return value;
}

void cst816_fill_status(SimStatus* out) {
    out->touch_down = g_down && !cst816_in_reset();
    out->touch_x = g_x;
    out->touch_y = g_y;
}
