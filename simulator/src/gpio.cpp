#include "gpio.h"

#include "log.h"

#include <cstdio>

namespace {

constexpr int kMaxPin = 48;

struct Pin {
    int mode;
    bool latch;
    bool ext;
    bool ext_set;
    bool pwm;
    int duty;
    int edge;
    void (*isr)();
    int isr_mode;
};

Pin g_pins[kMaxPin + 1];

bool in_range(int pin) {
    return pin >= 0 && pin <= kMaxPin;
}

}  // namespace

void gpio_init() {
    for (int i = 0; i <= kMaxPin; i++) {
        g_pins[i] = Pin{};
        g_pins[i].mode = GPIO_INPUT;
    }
}

bool gpio_valid(int pin) {
    return in_range(pin);
}

void gpio_pin_mode(int pin, int mode) {
    if (!in_range(pin)) {
        char key[32];
        std::snprintf(key, sizeof key, "badpin-%d", pin);
        sim_warn_once(key, "GPIO %d is outside 0..48", pin);
        return;
    }
    g_pins[pin].mode = mode;
    if (mode != GPIO_OUTPUT) {
        g_pins[pin].pwm = false;
    }
}

void gpio_write(int pin, bool high) {
    if (!in_range(pin)) {
        return;
    }
    Pin& p = g_pins[pin];
    if (p.mode != GPIO_OUTPUT) {
        char key[32];
        std::snprintf(key, sizeof key, "write-in-%d", pin);
        sim_warn_once(key, "digitalWrite(GPIO%d) before pinMode OUTPUT; level ignored", pin);
        return;
    }
    p.pwm = false;
    p.latch = high;
    p.duty = high ? 255 : 0;
}

bool gpio_read(int pin) {
    if (!in_range(pin)) {
        return false;
    }
    const Pin& p = g_pins[pin];
    if (p.mode == GPIO_OUTPUT) {
        return p.latch;
    }
    if (p.ext_set) {
        return p.ext;
    }
    return p.mode != GPIO_INPUT_PULLDOWN;
}

bool gpio_is_output(int pin) {
    return in_range(pin) && g_pins[pin].mode == GPIO_OUTPUT;
}

void gpio_set_pwm(int pin, int duty255) {
    if (!in_range(pin)) {
        return;
    }
    if (duty255 < 0) {
        duty255 = 0;
    }
    if (duty255 > 255) {
        duty255 = 255;
    }
    Pin& p = g_pins[pin];
    p.mode = GPIO_OUTPUT;
    p.pwm = true;
    p.duty = duty255;
    p.latch = duty255 > 0;
}

int gpio_duty(int pin) {
    if (!gpio_is_output(pin)) {
        return 0;
    }
    const Pin& p = g_pins[pin];
    if (p.pwm) {
        return p.duty;
    }
    return p.latch ? 255 : 0;
}

void gpio_set_input_level(int pin, bool high) {
    if (!in_range(pin)) {
        return;
    }
    Pin& p = g_pins[pin];
    if (p.ext_set && p.ext == high) {
        return;
    }
    bool was = p.ext_set ? p.ext : true;
    p.ext = high;
    p.ext_set = true;
    if (!was && high) {
        p.edge = 1;
    } else if (was && !high) {
        p.edge = -1;
    }
}

void gpio_attach(int pin, void (*fn)(), int mode) {
    if (!in_range(pin) || fn == nullptr) {
        return;
    }
    g_pins[pin].isr = fn;
    g_pins[pin].isr_mode = mode;
}

void gpio_detach(int pin) {
    if (!in_range(pin)) {
        return;
    }
    g_pins[pin].isr = nullptr;
}

void gpio_fire_isrs() {
    for (int pin = 0; pin <= kMaxPin; pin++) {
        Pin& p = g_pins[pin];
        if (p.edge == 0 || p.isr == nullptr) {
            continue;
        }
        int edge = p.edge;
        void (*fn)() = p.isr;
        int mode = p.isr_mode;
        p.edge = 0;
        bool fire = (mode == 0x03) || (mode == 0x01 && edge > 0) || (mode == 0x02 && edge < 0);
        if (fire) {
            fn();
        }
    }
}
