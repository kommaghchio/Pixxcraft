#pragma once

// Function table the loaded sketch uses to reach the simulator.
// The sketch DLL and this executable are separate binaries, so the sketch
// never links the simulator. It calls these pointers instead.

#include <stddef.h>
#include <stdint.h>

extern "C" {

struct PixxApi {
    void (*pin_mode)(uint8_t pin, uint8_t mode);
    void (*digital_write)(uint8_t pin, uint8_t val);
    int (*digital_read)(uint8_t pin);
    void (*analog_write)(uint8_t pin, int duty);
    bool (*ledc_attach)(uint8_t pin, uint32_t freq, uint8_t resolution);
    bool (*ledc_write)(uint8_t pin, uint32_t duty);
    bool (*ledc_detach)(uint8_t pin);
    void (*attach_interrupt)(uint8_t pin, void (*fn)(), int mode);
    void (*detach_interrupt)(uint8_t pin);
    void (*delay_ms)(uint32_t ms);
    void (*delay_us)(unsigned int us);
    uint32_t (*millis)(void);
    uint32_t (*micros)(void);
    void (*yield)(void);
    long (*random_max)(long max_value);
    long (*random_range)(long min_value, long max_value);
    void (*random_seed)(unsigned long seed);
    long (*map_value)(long x, long in_min, long in_max, long out_min, long out_max);
    void (*serial_begin)(unsigned long baud);
    size_t (*serial_write_byte)(uint8_t c);
    size_t (*serial_write_buf)(const uint8_t* buf, size_t len);
    void (*serial_flush)(void);
    int (*serial_available)(void);
    int (*serial_read)(void);
    void (*serial_print_str)(const char* s);
    void (*serial_print_char)(char c);
    void (*serial_print_int)(int v, int base);
    void (*serial_print_uint)(unsigned v, int base);
    void (*serial_print_long)(long v, int base);
    void (*serial_print_ulong)(unsigned long v, int base);
    void (*serial_print_ll)(long long v, int base);
    void (*serial_print_ull)(unsigned long long v, int base);
    void (*serial_print_double)(double v, int digits);
    void (*spi_begin)(int8_t sck, int8_t miso, int8_t mosi, int8_t ss);
    void (*spi_end)(void);
    void (*spi_begin_transaction)(uint32_t clock, uint8_t bit_order, uint8_t data_mode);
    void (*spi_end_transaction)(void);
    void (*spi_set_frequency)(uint32_t freq);
    void (*spi_set_bit_order)(uint8_t order);
    void (*spi_set_data_mode)(uint8_t mode);
    uint8_t (*spi_transfer)(uint8_t data);
    void (*spi_transfer_buf)(void* buf, size_t count);
    void (*spi_transfer_bytes)(const uint8_t* data, uint8_t* out, uint32_t size);
    void (*spi_write)(uint8_t data);
    void (*spi_write_bytes)(const uint8_t* data, uint32_t size);
    bool (*wire_begin)(int sda, int scl, uint32_t frequency);
    void (*wire_end)(void);
    void (*wire_set_clock)(uint32_t frequency);
    void (*wire_begin_transmission)(uint8_t address);
    uint8_t (*wire_end_transmission)(uint8_t send_stop);
    size_t (*wire_write_byte)(uint8_t data);
    size_t (*wire_write_buf)(const uint8_t* data, size_t len);
    uint8_t (*wire_request_from)(uint8_t address, uint8_t quantity, uint8_t send_stop);
    int (*wire_available)(void);
    int (*wire_read)(void);
    int (*wire_peek)(void);
};

const PixxApi* pixxcraft_api(void);

}
