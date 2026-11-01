#pragma once

#include <string.h>

class __FlashStringHelper {};

// Enough of Arduino's String for library methods that take one.
class String {
public:
    String() { buf_[0] = 0; }
    String(const char* text) { assign(text); }
    String(char c) {
        buf_[0] = c;
        buf_[1] = 0;
    }

    unsigned int length() const { return (unsigned int)strlen(buf_); }
    const char* c_str() const { return buf_; }

private:
    void assign(const char* text) {
        if (text == nullptr) {
            buf_[0] = 0;
            return;
        }
        strncpy(buf_, text, sizeof buf_ - 1);
        buf_[sizeof buf_ - 1] = 0;
    }

    char buf_[160];
};
