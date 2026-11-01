#pragma once

#include <stdbool.h>

enum {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
    GPIO_INPUT_PULLUP = 2,
    GPIO_INPUT_PULLDOWN = 3
};

void gpio_init();
bool gpio_valid(int pin);
void gpio_pin_mode(int pin, int mode);
void gpio_write(int pin, bool high);
bool gpio_read(int pin);
bool gpio_is_output(int pin);
void gpio_set_pwm(int pin, int duty255);
int gpio_duty(int pin);
void gpio_set_input_level(int pin, bool high);
void gpio_attach(int pin, void (*fn)(), int mode);
void gpio_detach(int pin);
void gpio_fire_isrs();
