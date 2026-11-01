// Compiled into each loaded sketch. Arduino calls land here, then jump
// through pixxcraft_api() into the simulator process.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdarg>
#include <cstdio>

#include "pixx_api.h"
#include "Arduino.h"
#include "SPI.h"
#include "Wire.h"

namespace {

const PixxApi* api() {
    static const PixxApi* cached = nullptr;
    if (cached == nullptr) {
        HMODULE exe = GetModuleHandleW(nullptr);
        auto lookup = reinterpret_cast<const PixxApi* (*)()>(GetProcAddress(exe, "pixxcraft_api"));
        cached = lookup != nullptr ? lookup() : nullptr;
        if (cached == nullptr) {
            std::fputs("sketch cannot find pixxcraft_api in the simulator\n", stderr);
        }
    }
    return cached;
}

}  // namespace

void pinMode(uint8_t pin, uint8_t mode) {
    if (const PixxApi* a = api()) a->pin_mode(pin, mode);
}
void digitalWrite(uint8_t pin, uint8_t val) {
    if (const PixxApi* a = api()) a->digital_write(pin, val);
}
int digitalRead(uint8_t pin) {
    const PixxApi* a = api();
    return a != nullptr ? a->digital_read(pin) : 0;
}
void analogWrite(uint8_t pin, int duty) {
    if (const PixxApi* a = api()) a->analog_write(pin, duty);
}
bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {
    const PixxApi* a = api();
    return a != nullptr && a->ledc_attach(pin, freq, resolution);
}
bool ledcWrite(uint8_t pin, uint32_t duty) {
    const PixxApi* a = api();
    return a != nullptr && a->ledc_write(pin, duty);
}
bool ledcDetach(uint8_t pin) {
    const PixxApi* a = api();
    return a != nullptr && a->ledc_detach(pin);
}
void attachInterrupt(uint8_t pin, void (*fn)(), int mode) {
    if (const PixxApi* a = api()) a->attach_interrupt(pin, fn, mode);
}
void detachInterrupt(uint8_t pin) {
    if (const PixxApi* a = api()) a->detach_interrupt(pin);
}
void delay(uint32_t ms) {
    if (const PixxApi* a = api()) a->delay_ms(ms);
}
void delayMicroseconds(unsigned int us) {
    if (const PixxApi* a = api()) a->delay_us(us);
}
uint32_t millis() {
    const PixxApi* a = api();
    return a != nullptr ? a->millis() : 0;
}
uint32_t micros() {
    const PixxApi* a = api();
    return a != nullptr ? a->micros() : 0;
}
void yield() {
    if (const PixxApi* a = api()) a->yield();
}
long random(long max_value) {
    const PixxApi* a = api();
    return a != nullptr ? a->random_max(max_value) : 0;
}
long random(long min_value, long max_value) {
    const PixxApi* a = api();
    return a != nullptr ? a->random_range(min_value, max_value) : min_value;
}
void randomSeed(unsigned long seed) {
    if (const PixxApi* a = api()) a->random_seed(seed);
}
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const PixxApi* a = api();
    return a != nullptr ? a->map_value(x, in_min, in_max, out_min, out_max) : out_min;
}

void SerialClass::begin(unsigned long baud) {
    if (const PixxApi* a = api()) a->serial_begin(baud);
}
size_t SerialClass::write(uint8_t c) {
    const PixxApi* a = api();
    return a != nullptr ? a->serial_write_byte(c) : 0;
}
size_t SerialClass::write(const uint8_t* buf, size_t len) {
    const PixxApi* a = api();
    return a != nullptr ? a->serial_write_buf(buf, len) : 0;
}
void SerialClass::flush() {
    if (const PixxApi* a = api()) a->serial_flush();
}
int SerialClass::available() {
    const PixxApi* a = api();
    return a != nullptr ? a->serial_available() : 0;
}
int SerialClass::read() {
    const PixxApi* a = api();
    return a != nullptr ? a->serial_read() : -1;
}
void SerialClass::printf(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    print(buf);
}
void SerialClass::print(const char* s) {
    if (const PixxApi* a = api()) a->serial_print_str(s);
}
void SerialClass::print(char c) {
    if (const PixxApi* a = api()) a->serial_print_char(c);
}
void SerialClass::print(int v, int base) {
    if (const PixxApi* a = api()) a->serial_print_int(v, base);
}
void SerialClass::print(unsigned v, int base) {
    if (const PixxApi* a = api()) a->serial_print_uint(v, base);
}
void SerialClass::print(long v, int base) {
    if (const PixxApi* a = api()) a->serial_print_long(v, base);
}
void SerialClass::print(unsigned long v, int base) {
    if (const PixxApi* a = api()) a->serial_print_ulong(v, base);
}
void SerialClass::print(long long v, int base) {
    if (const PixxApi* a = api()) a->serial_print_ll(v, base);
}
void SerialClass::print(unsigned long long v, int base) {
    if (const PixxApi* a = api()) a->serial_print_ull(v, base);
}
void SerialClass::print(double v, int digits) {
    if (const PixxApi* a = api()) a->serial_print_double(v, digits);
}
SerialClass Serial;

SPISettings::SPISettings() : clock(1000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {}
SPISettings::SPISettings(uint32_t clock_hz, uint8_t order, uint8_t mode)
    : clock(clock_hz), bitOrder(order), dataMode(mode) {}

void SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) {
    if (const PixxApi* a = api()) a->spi_begin(sck, miso, mosi, ss);
}
void SPIClass::end() {
    if (const PixxApi* a = api()) a->spi_end();
}
void SPIClass::beginTransaction(SPISettings settings) {
    if (const PixxApi* a = api()) a->spi_begin_transaction(settings.clock, settings.bitOrder, settings.dataMode);
}
void SPIClass::endTransaction() {
    if (const PixxApi* a = api()) a->spi_end_transaction();
}
void SPIClass::setFrequency(uint32_t freq) {
    if (const PixxApi* a = api()) a->spi_set_frequency(freq);
}
void SPIClass::setBitOrder(uint8_t order) {
    if (const PixxApi* a = api()) a->spi_set_bit_order(order);
}
void SPIClass::setDataMode(uint8_t mode) {
    if (const PixxApi* a = api()) a->spi_set_data_mode(mode);
}
uint8_t SPIClass::transfer(uint8_t data) {
    const PixxApi* a = api();
    return a != nullptr ? a->spi_transfer(data) : 0xFF;
}
void SPIClass::transfer(void* buf, size_t count) {
    if (const PixxApi* a = api()) a->spi_transfer_buf(buf, count);
}
void SPIClass::transferBytes(const uint8_t* data, uint8_t* out, uint32_t size) {
    if (const PixxApi* a = api()) a->spi_transfer_bytes(data, out, size);
}
void SPIClass::write(uint8_t data) {
    if (const PixxApi* a = api()) a->spi_write(data);
}
void SPIClass::writeBytes(const uint8_t* data, uint32_t size) {
    if (const PixxApi* a = api()) a->spi_write_bytes(data, size);
}
SPIClass SPI;

bool TwoWire::begin(int sda, int scl, uint32_t frequency) {
    const PixxApi* a = api();
    return a != nullptr && a->wire_begin(sda, scl, frequency);
}
void TwoWire::end() {
    if (const PixxApi* a = api()) a->wire_end();
}
void TwoWire::setClock(uint32_t frequency) {
    if (const PixxApi* a = api()) a->wire_set_clock(frequency);
}
void TwoWire::beginTransmission(uint8_t address) {
    if (const PixxApi* a = api()) a->wire_begin_transmission(address);
}
uint8_t TwoWire::endTransmission(uint8_t sendStop) {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_end_transmission(sendStop) : 2;
}
size_t TwoWire::write(uint8_t data) {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_write_byte(data) : 0;
}
size_t TwoWire::write(const uint8_t* data, size_t len) {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_write_buf(data, len) : 0;
}
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_request_from(address, quantity, sendStop) : 0;
}
int TwoWire::available() {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_available() : 0;
}
int TwoWire::read() {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_read() : -1;
}
int TwoWire::peek() {
    const PixxApi* a = api();
    return a != nullptr ? a->wire_peek() : -1;
}
TwoWire Wire;
