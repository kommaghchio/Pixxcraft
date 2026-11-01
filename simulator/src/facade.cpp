#include "sim_api.h"

#include "board.h"
#include "bus.h"
#include "cst816.h"
#include "gpio.h"
#include "log.h"
#include "st7789.h"

#include <cstdio>
#include <cstring>

void sim_reset_board() {
    gpio_init();
    st7789_init();
    cst816_init();
    bus_reset();
    sim_log_clear();
}

void sim_collect_status(SimStatus* out) {
    std::memset(out, 0, sizeof *out);
    st7789_fill_status(out);
    cst816_fill_status(out);
    bus_fill_status(out);
    out->led_on = gpio_is_output(kPinLed) && gpio_read(kPinLed);
}

uint32_t sim_glass_rgb(int x, int y) {
    return st7789_glass_rgb(x, y);
}

const char* sim_blank_reason() {
    static SimStatus status;
    sim_collect_status(&status);
    return status.blank;
}

bool sim_save_bmp(const char* path) {
    if (path == nullptr || path[0] == 0) {
        return false;
    }
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    const int width = kGlassW;
    const int height = kGlassH;
    const int row_stride = (width * 3 + 3) & ~3;
    const uint32_t pixel_bytes = (uint32_t)row_stride * (uint32_t)height;
    const uint32_t file_size = 54u + pixel_bytes;
    uint8_t header[54] = {};
    header[0] = 'B';
    header[1] = 'M';
    header[2] = (uint8_t)(file_size);
    header[3] = (uint8_t)(file_size >> 8);
    header[4] = (uint8_t)(file_size >> 16);
    header[5] = (uint8_t)(file_size >> 24);
    header[10] = 54;
    header[14] = 40;
    header[18] = (uint8_t)width;
    header[19] = (uint8_t)(width >> 8);
    header[22] = (uint8_t)height;
    header[23] = (uint8_t)(height >> 8);
    header[26] = 1;
    header[28] = 24;
    if (std::fwrite(header, 1, 54, file) != 54) {
        std::fclose(file);
        return false;
    }
    uint8_t row[240 * 3 + 4];
    for (int y = height - 1; y >= 0; y--) {
        std::memset(row, 0, (size_t)row_stride);
        for (int x = 0; x < width; x++) {
            uint32_t color = sim_glass_rgb(x, y);
            row[x * 3 + 0] = (uint8_t)(color);
            row[x * 3 + 1] = (uint8_t)(color >> 8);
            row[x * 3 + 2] = (uint8_t)(color >> 16);
        }
        if (std::fwrite(row, 1, (size_t)row_stride, file) != (size_t)row_stride) {
            std::fclose(file);
            return false;
        }
    }
    std::fclose(file);
    return true;
}

void sim_force_touch(int x, int y, bool down) {
    cst816_force_touch(x, y, down);
}

void sim_release_touch_override() {
    cst816_clear_override();
}

bool sim_touch_overridden() {
    return cst816_touch_overridden();
}
