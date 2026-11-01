#pragma once

#include "sim_api.h"

#include <stddef.h>
#include <stdint.h>

void cst816_init();
bool cst816_in_reset();
bool cst816_touch_overridden();
void cst816_force_touch(int x, int y, bool down);
void cst816_clear_override();
void cst816_apply_mouse(int x, int y, bool down);
void cst816_set_pointer(uint8_t reg);
void cst816_write(uint8_t reg, const uint8_t* data, size_t len);
uint8_t cst816_read_next();
void cst816_fill_status(SimStatus* out);
