#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class Print {
public:
    virtual size_t write(uint8_t value) = 0;

    virtual size_t write(const uint8_t* buffer, size_t size) {
        size_t written = 0;
        while (size-- > 0) {
            if (write(*buffer++) == 0) {
                break;
            }
            written++;
        }
        return written;
    }

    size_t write(const char* text) {
        if (text == nullptr) {
            return 0;
        }
        return write(reinterpret_cast<const uint8_t*>(text), strlen(text));
    }

    size_t print(const char* text) { return write(text); }
    size_t print(char c) { return write(static_cast<uint8_t>(c)); }
    size_t print(unsigned long value, int base = 10) {
        char buf[40];
        if (base == 16) {
            snprintf(buf, sizeof buf, "%lX", value);
        } else if (base == 8) {
            snprintf(buf, sizeof buf, "%lo", value);
        } else {
            snprintf(buf, sizeof buf, "%lu", value);
        }
        return print(buf);
    }
    size_t print(unsigned value, int base = 10) { return print(static_cast<unsigned long>(value), base); }
    size_t print(long value, int base = 10) {
        if (value < 0 && base == 10) {
            return write(static_cast<uint8_t>('-')) + print(static_cast<unsigned long>(-value), base);
        }
        return print(static_cast<unsigned long>(value), base);
    }
    size_t print(int value, int base = 10) { return print(static_cast<long>(value), base); }
    size_t println() { return write(static_cast<uint8_t>('\n')); }
    size_t println(const char* text) { return print(text) + println(); }
};

class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() {}
};
