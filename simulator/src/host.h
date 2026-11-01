#pragma once

#include <stdbool.h>
#include <stdint.h>

bool host_init(int scale);
void host_shutdown();
void host_pump();
void host_present();
void host_delay(uint32_t ms);
uint32_t host_millis();
uint32_t host_micros();
bool host_quit();
bool host_capture(const char* path);
bool host_ready();
void host_set_title(const char* title);
void host_enable_ui(bool enabled);
