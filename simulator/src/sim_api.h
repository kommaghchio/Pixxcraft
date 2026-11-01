#pragma once

#include <stdbool.h>
#include <stdint.h>

struct SimStatus {
    int backlight;
    bool reset_held;
    bool sleep;
    bool display_on;
    bool inverted;
    bool cs_selected;
    bool dc_data;
    uint8_t madctl;
    uint8_t colmod;
    int xs, xe, ys, ye;
    unsigned pixels_in;
    unsigned pixels_out;
    bool spi_started;
    bool spi_on_lcd;
    int spi_sck;
    int spi_mosi;
    int spi_mode;
    bool i2c_started;
    bool i2c_on_tp;
    int i2c_sda;
    int i2c_scl;
    bool touch_down;
    int touch_x;
    int touch_y;
    bool led_on;
    char blank[320];
    char last_cmd[48];
};

void sim_reset_board();
void sim_collect_status(SimStatus* out);
uint32_t sim_glass_rgb(int x, int y);
const char* sim_blank_reason();
bool sim_save_bmp(const char* path);

void sim_force_touch(int x, int y, bool down);
void sim_release_touch_override();
bool sim_touch_overridden();

int run_self_test(const char* dump_path);
