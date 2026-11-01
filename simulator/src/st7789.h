#pragma once

#include "sim_api.h"

#include <stdint.h>

void st7789_init();
void st7789_observe_reset();
void st7789_spi_byte(uint8_t data);
uint32_t st7789_glass_rgb(int x, int y);
void st7789_fill_status(SimStatus* out);
