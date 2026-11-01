#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "Arduino.h"
#undef min
#undef max
#include "SPI.h"
#include "Wire.h"

#include "board.h"
#include "bus.h"
#include "cst816.h"
#include "gpio.h"
#include "host.h"
#include "log.h"
#include "st7789.h"

namespace {

int map_mode(uint8_t mode) {
    if (mode == OUTPUT) {
        return GPIO_OUTPUT;
    }
    if (mode == INPUT_PULLUP) {
        return GPIO_INPUT_PULLUP;
    }
    if (mode == INPUT_PULLDOWN) {
        return GPIO_INPUT_PULLDOWN;
    }
    return GPIO_INPUT;
}

uint8_t reverse_bits(uint8_t value) {
    value = (uint8_t)((value & 0xF0) >> 4 | (value & 0x0F) << 4);
    value = (uint8_t)((value & 0xCC) >> 2 | (value & 0x33) << 2);
    value = (uint8_t)((value & 0xAA) >> 1 | (value & 0x55) << 1);
    return value;
}

int g_ledc_res[49] = {};

struct SpiState {
    bool started = false;
    int sck = 7;
    int miso = 8;
    int mosi = 9;
    int ss = 44;
    uint32_t clock = 1000000;
    uint8_t bit_order = MSBFIRST;
    uint8_t data_mode = SPI_MODE0;
    int warned_sck = -999;
    int warned_mosi = -999;
    int xfer_count = 0;
} g_spi;

void note_spi_pins() {
    if (g_spi.sck == kPinSck && g_spi.mosi == kPinMosi) {
        g_spi.warned_sck = -999;
        g_spi.warned_mosi = -999;
        return;
    }
    if (g_spi.sck == g_spi.warned_sck && g_spi.mosi == g_spi.warned_mosi) {
        return;
    }
    sim_log_warn(
        "SPI is on SCK GPIO%d MOSI GPIO%d. The LCD clock is GPIO7 and data is GPIO9.",
        g_spi.sck,
        g_spi.mosi);
    g_spi.warned_sck = g_spi.sck;
    g_spi.warned_mosi = g_spi.mosi;
}

struct WireState {
    bool started = false;
    int sda = 5;
    int scl = 6;
    uint32_t frequency = 400000;
    uint8_t addr = 0;
    bool open = false;
    std::vector<uint8_t> tx;
    std::vector<uint8_t> rx;
    size_t rx_i = 0;
} g_wire;

bool wire_bus_ok() {
    return g_wire.started && g_wire.sda == kPinTpSda && g_wire.scl == kPinTpScl;
}

uint8_t wire_check(uint8_t address) {
    if (!g_wire.started) {
        sim_warn_once("wire-begin", "Wire transfer before Wire.begin()");
        return 2;
    }
    if (!wire_bus_ok()) {
        sim_warn_once(
            "wire-pins",
            "Wire is on SDA GPIO%d SCL GPIO%d. The touch controller is SDA GPIO5 SCL GPIO6.",
            g_wire.sda,
            g_wire.scl);
        return 2;
    }
    if (cst816_in_reset()) {
        sim_warn_once("wire-rst", "touch I2C NACK: TP_RST GPIO44 is held low");
        return 2;
    }
    if (address == 0x2A || address == 0x2B) {
        sim_warn_once("wire-8bit", "I2C address 0x%02X is the 8-bit form. Wire wants the 7-bit address 0x15.", address);
        return 2;
    }
    if (address != 0x15) {
        char key[24];
        std::snprintf(key, sizeof key, "wire-addr-%02X", address);
        sim_warn_once(key, "I2C address 0x%02X NACK. CST816 is 7-bit 0x15.", address);
        return 2;
    }
    return 0;
}

void print_ull(unsigned long long value, int base) {
    if (base < 2 || base > 16) {
        base = 10;
    }
    char buf[72];
    if (value == 0) {
        std::fputc('0', stdout);
        return;
    }
    int n = 0;
    while (value > 0 && n < 70) {
        buf[n++] = "0123456789ABCDEF"[value % (unsigned)base];
        value /= (unsigned)base;
    }
    while (n > 0) {
        std::fputc(buf[--n], stdout);
    }
}

}  // namespace

void pinMode(uint8_t pin, uint8_t mode) {
    gpio_pin_mode(pin, map_mode(mode));
    st7789_observe_reset();
}

void digitalWrite(uint8_t pin, uint8_t val) {
    gpio_write(pin, val != 0);
    st7789_observe_reset();
}

int digitalRead(uint8_t pin) {
    return gpio_read(pin) ? HIGH : LOW;
}

void analogWrite(uint8_t pin, int duty) {
    gpio_set_pwm(pin, duty);
}

bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {
    (void)freq;
    if (pin > 48) {
        return false;
    }
    if (resolution < 1) {
        resolution = 1;
    }
    if (resolution > 16) {
        resolution = 16;
    }
    g_ledc_res[pin] = resolution;
    gpio_pin_mode(pin, GPIO_OUTPUT);
    return true;
}

bool ledcWrite(uint8_t pin, uint32_t duty) {
    if (pin > 48) {
        return false;
    }
    int bits = g_ledc_res[pin] != 0 ? g_ledc_res[pin] : 8;
    uint32_t max_duty = (1u << bits) - 1u;
    if (duty > max_duty) {
        duty = max_duty;
    }
    int duty255 = (int)((duty * 255ull) / max_duty);
    gpio_set_pwm(pin, duty255);
    return true;
}

bool ledcDetach(uint8_t pin) {
    if (pin > 48) {
        return false;
    }
    g_ledc_res[pin] = 0;
    gpio_write(pin, false);
    return true;
}

void attachInterrupt(uint8_t pin, void (*fn)(), int mode) {
    gpio_attach(pin, fn, mode);
}

void detachInterrupt(uint8_t pin) {
    gpio_detach(pin);
}

void delay(uint32_t ms) {
    host_delay(ms);
}

void delayMicroseconds(unsigned int us) {
    if (us >= 2000) {
        delay(us / 1000);
        return;
    }
    uint32_t start = micros();
    while ((micros() - start) < us) {
    }
}

uint32_t millis() {
    return host_millis();
}

uint32_t micros() {
    return host_micros();
}

void yield() {
    host_pump();
}

long random(long max) {
    if (max <= 0) {
        return 0;
    }
    return std::rand() % max;
}

long random(long min_value, long max_value) {
    if (max_value <= min_value) {
        return min_value;
    }
    return min_value + random(max_value - min_value);
}

void randomSeed(unsigned long seed) {
    std::srand((unsigned)seed);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) {
        return out_min;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void SerialClass::begin(unsigned long baud) {
    (void)baud;
    sim_info_once(
        "serial",
        "Serial goes to this console. On the XIAO, GPIO43 and GPIO44 are touch INT and RST.");
}

size_t SerialClass::write(uint8_t c) {
    std::fputc(c, stdout);
    return 1;
}

size_t SerialClass::write(const uint8_t* buf, size_t len) {
    if (buf == nullptr || len == 0) {
        return 0;
    }
    return std::fwrite(buf, 1, len, stdout);
}

void SerialClass::flush() {
    std::fflush(stdout);
}

int SerialClass::available() {
    return 0;
}

int SerialClass::read() {
    return -1;
}

void SerialClass::printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void SerialClass::print(const char* s) {
    if (s != nullptr) {
        std::fputs(s, stdout);
    }
}

void SerialClass::print(char c) {
    write((uint8_t)c);
}

void SerialClass::print(int v, int base) {
    print(static_cast<long>(v), base);
}

void SerialClass::print(unsigned v, int base) {
    print(static_cast<unsigned long>(v), base);
}

void SerialClass::print(long v, int base) {
    if (base == DEC && v < 0) {
        write('-');
        print(static_cast<unsigned long>(-(v + 1)) + 1ul, DEC);
        return;
    }
    print(static_cast<unsigned long>(v), base);
}

void SerialClass::print(unsigned long v, int base) {
    print_ull(v, base);
}

void SerialClass::print(long long v, int base) {
    if (base == DEC && v < 0) {
        write('-');
        print(static_cast<unsigned long long>(-(v + 1)) + 1ull, DEC);
        return;
    }
    print(static_cast<unsigned long long>(v), base);
}

void SerialClass::print(unsigned long long v, int base) {
    print_ull(v, base);
}

void SerialClass::print(double v, int digits) {
    if (digits < 0) {
        digits = 0;
    }
    if (digits > 8) {
        digits = 8;
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", digits, v);
    print(buf);
}

SerialClass Serial;

SPISettings::SPISettings() : clock(1000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {}

SPISettings::SPISettings(uint32_t clock_hz, uint8_t order, uint8_t mode)
    : clock(clock_hz), bitOrder(order), dataMode(mode) {}

void SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) {
    g_spi.started = true;
    g_spi.sck = sck < 0 ? kPinSck : sck;
    g_spi.miso = miso < 0 ? kPinMiso : miso;
    g_spi.mosi = mosi < 0 ? kPinMosi : mosi;
    g_spi.ss = ss < 0 ? 44 : ss;
    note_spi_pins();
    if (g_spi.ss == kPinTpRst) {
        sim_info_once(
            "spi-ss",
            "SPI SS is GPIO44, the touch reset pin. transfer() does not toggle it. LCD CS is GPIO2.");
    }
}

void SPIClass::end() {
    g_spi.started = false;
}

void SPIClass::beginTransaction(SPISettings settings) {
    g_spi.clock = settings.clock;
    g_spi.bit_order = settings.bitOrder;
    g_spi.data_mode = settings.dataMode;
    if (settings.dataMode != SPI_MODE0) {
        sim_warn_once("spi-mode", "SPI mode %d. The ST7789 on this module needs SPI_MODE0.", settings.dataMode);
    }
    if (settings.bitOrder != MSBFIRST) {
        sim_warn_once("spi-lsb", "SPI bit order is LSB first. The ST7789 expects MSB first.");
    }
}

void SPIClass::endTransaction() {}

void SPIClass::setFrequency(uint32_t freq) {
    g_spi.clock = freq;
}

void SPIClass::setBitOrder(uint8_t order) {
    g_spi.bit_order = order;
}

void SPIClass::setDataMode(uint8_t mode) {
    g_spi.data_mode = mode;
}

uint8_t SPIClass::transfer(uint8_t data) {
    if (!g_spi.started) {
        sim_warn_once("spi-begin", "SPI.transfer before SPI.begin()");
        return 0xFF;
    }
    if (g_spi.data_mode != SPI_MODE0) {
        sim_warn_once("spi-mode", "SPI mode %d. The ST7789 on this module needs SPI_MODE0.", g_spi.data_mode);
    }
    note_spi_pins();
    uint8_t on_wire = g_spi.bit_order == LSBFIRST ? reverse_bits(data) : data;
    if (g_spi.sck == kPinSck && g_spi.mosi == kPinMosi) {
        st7789_spi_byte(on_wire);
    }
    if ((++g_spi.xfer_count & 255) == 0) {
        host_pump();
    }
    return 0xFF;
}

void SPIClass::transfer(void* buf, size_t count) {
    auto* bytes = static_cast<uint8_t*>(buf);
    for (size_t i = 0; i < count; i++) {
        bytes[i] = transfer(bytes[i]);
    }
}

void SPIClass::transferBytes(const uint8_t* data, uint8_t* out, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        uint8_t value = transfer(data != nullptr ? data[i] : 0);
        if (out != nullptr) {
            out[i] = value;
        }
    }
}

void SPIClass::write(uint8_t data) {
    transfer(data);
}

void SPIClass::writeBytes(const uint8_t* data, uint32_t size) {
    transferBytes(data, nullptr, size);
}

SPIClass SPI;

bool TwoWire::begin(int sda, int scl, uint32_t frequency) {
    g_wire.started = true;
    g_wire.sda = sda < 0 ? kPinTpSda : sda;
    g_wire.scl = scl < 0 ? kPinTpScl : scl;
    if (frequency != 0) {
        g_wire.frequency = frequency;
    }
    if (!wire_bus_ok()) {
        sim_warn_once(
            "wire-pins",
            "Wire.begin(SDA=%d, SCL=%d) does not reach the touch controller on GPIO5/GPIO6.",
            g_wire.sda,
            g_wire.scl);
    }
    return true;
}

void TwoWire::end() {
    g_wire.started = false;
}

void TwoWire::setClock(uint32_t frequency) {
    g_wire.frequency = frequency;
}

void TwoWire::beginTransmission(uint8_t address) {
    g_wire.addr = address;
    g_wire.open = true;
    g_wire.tx.clear();
}

uint8_t TwoWire::endTransmission(uint8_t sendStop) {
    (void)sendStop;
    uint8_t rc = wire_check(g_wire.addr);
    if (rc == 0 && g_wire.tx.size() == 1) {
        cst816_set_pointer(g_wire.tx[0]);
    } else if (rc == 0 && g_wire.tx.size() > 1) {
        cst816_write(g_wire.tx[0], g_wire.tx.data() + 1, g_wire.tx.size() - 1);
    }
    g_wire.tx.clear();
    g_wire.open = false;
    return rc;
}

size_t TwoWire::write(uint8_t data) {
    if (!g_wire.open) {
        sim_warn_once("wire-write", "Wire.write before beginTransmission");
        return 0;
    }
    g_wire.tx.push_back(data);
    return 1;
}

size_t TwoWire::write(const uint8_t* data, size_t len) {
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
        n += write(data[i]);
    }
    return n;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
    (void)sendStop;
    g_wire.rx.clear();
    g_wire.rx_i = 0;
    if (g_wire.open) {
        sim_warn_once("wire-open", "requestFrom while a transmission is still open. Call endTransmission first.");
    }
    if (wire_check(address) != 0) {
        return 0;
    }
    g_wire.rx.reserve(quantity);
    for (uint8_t i = 0; i < quantity; i++) {
        g_wire.rx.push_back(cst816_read_next());
    }
    return quantity;
}

int TwoWire::available() {
    if (g_wire.rx_i >= g_wire.rx.size()) {
        return 0;
    }
    return (int)(g_wire.rx.size() - g_wire.rx_i);
}

int TwoWire::read() {
    if (g_wire.rx_i >= g_wire.rx.size()) {
        return -1;
    }
    return g_wire.rx[g_wire.rx_i++];
}

int TwoWire::peek() {
    if (g_wire.rx_i >= g_wire.rx.size()) {
        return -1;
    }
    return g_wire.rx[g_wire.rx_i];
}

TwoWire Wire;

void bus_reset() {
    g_spi = SpiState{};
    g_wire = WireState{};
    std::memset(g_ledc_res, 0, sizeof g_ledc_res);
}

void bus_fill_status(SimStatus* out) {
    out->spi_started = g_spi.started;
    out->spi_sck = g_spi.sck;
    out->spi_mosi = g_spi.mosi;
    out->spi_mode = g_spi.data_mode;
    out->spi_on_lcd = g_spi.started && g_spi.sck == kPinSck && g_spi.mosi == kPinMosi;
    out->i2c_started = g_wire.started;
    out->i2c_sda = g_wire.sda;
    out->i2c_scl = g_wire.scl;
    out->i2c_on_tp = wire_bus_ok();
}
