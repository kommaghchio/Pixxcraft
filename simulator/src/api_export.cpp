#include "pixx_api.h"

#include "Arduino.h"
#include "SPI.h"
#include "Wire.h"

namespace {

void api_pin_mode(uint8_t pin, uint8_t mode) { pinMode(pin, mode); }
void api_digital_write(uint8_t pin, uint8_t val) { digitalWrite(pin, val); }
int api_digital_read(uint8_t pin) { return digitalRead(pin); }
void api_analog_write(uint8_t pin, int duty) { analogWrite(pin, duty); }
bool api_ledc_attach(uint8_t pin, uint32_t freq, uint8_t resolution) { return ledcAttach(pin, freq, resolution); }
bool api_ledc_write(uint8_t pin, uint32_t duty) { return ledcWrite(pin, duty); }
bool api_ledc_detach(uint8_t pin) { return ledcDetach(pin); }
void api_attach_interrupt(uint8_t pin, void (*fn)(), int mode) { attachInterrupt(pin, fn, mode); }
void api_detach_interrupt(uint8_t pin) { detachInterrupt(pin); }
void api_delay_ms(uint32_t ms) { delay(ms); }
void api_delay_us(unsigned int us) { delayMicroseconds(us); }
uint32_t api_millis() { return millis(); }
uint32_t api_micros() { return micros(); }
void api_yield() { yield(); }
long api_random_max(long max_value) { return random(max_value); }
long api_random_range(long min_value, long max_value) { return random(min_value, max_value); }
void api_random_seed(unsigned long seed) { randomSeed(seed); }
long api_map(long x, long in_min, long in_max, long out_min, long out_max) {
    return map(x, in_min, in_max, out_min, out_max);
}
void api_serial_begin(unsigned long baud) { Serial.begin(baud); }
size_t api_serial_write_byte(uint8_t c) { return Serial.write(c); }
size_t api_serial_write_buf(const uint8_t* buf, size_t len) { return Serial.write(buf, len); }
void api_serial_flush() { Serial.flush(); }
int api_serial_available() { return Serial.available(); }
int api_serial_read() { return Serial.read(); }
void api_serial_print_str(const char* s) { Serial.print(s); }
void api_serial_print_char(char c) { Serial.print(c); }
void api_serial_print_int(int v, int base) { Serial.print(v, base); }
void api_serial_print_uint(unsigned v, int base) { Serial.print(v, base); }
void api_serial_print_long(long v, int base) { Serial.print(v, base); }
void api_serial_print_ulong(unsigned long v, int base) { Serial.print(v, base); }
void api_serial_print_ll(long long v, int base) { Serial.print(v, base); }
void api_serial_print_ull(unsigned long long v, int base) { Serial.print(v, base); }
void api_serial_print_double(double v, int digits) { Serial.print(v, digits); }
void api_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) { SPI.begin(sck, miso, mosi, ss); }
void api_spi_end() { SPI.end(); }
void api_spi_begin_transaction(uint32_t clock, uint8_t bit_order, uint8_t data_mode) {
    SPI.beginTransaction(SPISettings(clock, bit_order, data_mode));
}
void api_spi_end_transaction() { SPI.endTransaction(); }
void api_spi_set_frequency(uint32_t freq) { SPI.setFrequency(freq); }
void api_spi_set_bit_order(uint8_t order) { SPI.setBitOrder(order); }
void api_spi_set_data_mode(uint8_t mode) { SPI.setDataMode(mode); }
uint8_t api_spi_transfer(uint8_t data) { return SPI.transfer(data); }
void api_spi_transfer_buf(void* buf, size_t count) { SPI.transfer(buf, count); }
void api_spi_transfer_bytes(const uint8_t* data, uint8_t* out, uint32_t size) { SPI.transferBytes(data, out, size); }
void api_spi_write(uint8_t data) { SPI.write(data); }
void api_spi_write_bytes(const uint8_t* data, uint32_t size) { SPI.writeBytes(data, size); }
bool api_wire_begin(int sda, int scl, uint32_t frequency) { return Wire.begin(sda, scl, frequency); }
void api_wire_end() { Wire.end(); }
void api_wire_set_clock(uint32_t frequency) { Wire.setClock(frequency); }
void api_wire_begin_transmission(uint8_t address) { Wire.beginTransmission(address); }
uint8_t api_wire_end_transmission(uint8_t send_stop) { return Wire.endTransmission(send_stop); }
size_t api_wire_write_byte(uint8_t data) { return Wire.write(data); }
size_t api_wire_write_buf(const uint8_t* data, size_t len) { return Wire.write(data, len); }
uint8_t api_wire_request_from(uint8_t address, uint8_t quantity, uint8_t send_stop) {
    return Wire.requestFrom(address, quantity, send_stop);
}
int api_wire_available() { return Wire.available(); }
int api_wire_read() { return Wire.read(); }
int api_wire_peek() { return Wire.peek(); }

const PixxApi g_api = {
    api_pin_mode,
    api_digital_write,
    api_digital_read,
    api_analog_write,
    api_ledc_attach,
    api_ledc_write,
    api_ledc_detach,
    api_attach_interrupt,
    api_detach_interrupt,
    api_delay_ms,
    api_delay_us,
    api_millis,
    api_micros,
    api_yield,
    api_random_max,
    api_random_range,
    api_random_seed,
    api_map,
    api_serial_begin,
    api_serial_write_byte,
    api_serial_write_buf,
    api_serial_flush,
    api_serial_available,
    api_serial_read,
    api_serial_print_str,
    api_serial_print_char,
    api_serial_print_int,
    api_serial_print_uint,
    api_serial_print_long,
    api_serial_print_ulong,
    api_serial_print_ll,
    api_serial_print_ull,
    api_serial_print_double,
    api_spi_begin,
    api_spi_end,
    api_spi_begin_transaction,
    api_spi_end_transaction,
    api_spi_set_frequency,
    api_spi_set_bit_order,
    api_spi_set_data_mode,
    api_spi_transfer,
    api_spi_transfer_buf,
    api_spi_transfer_bytes,
    api_spi_write,
    api_spi_write_bytes,
    api_wire_begin,
    api_wire_end,
    api_wire_set_clock,
    api_wire_begin_transmission,
    api_wire_end_transmission,
    api_wire_write_byte,
    api_wire_write_buf,
    api_wire_request_from,
    api_wire_available,
    api_wire_read,
    api_wire_peek,
};

}  // namespace

extern "C" __declspec(dllexport) const PixxApi* pixxcraft_api(void) {
    return &g_api;
}
