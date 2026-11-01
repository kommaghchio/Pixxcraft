#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>

namespace {

constexpr int kCap = 160;
constexpr int kLen = 1024;

struct Line {
    int level;
    char text[kLen];
};

Line g_lines[kCap];
int g_next = 0;
int g_filled = 0;
int g_warnings = 0;
std::unordered_set<std::string> g_once;

void push(int level, const char* text) {
    Line& line = g_lines[g_next];
    line.level = level;
    std::snprintf(line.text, kLen, "%s", text);
    g_next = (g_next + 1) % kCap;
    if (g_filled < kCap) {
        g_filled++;
    }
    std::fputs(level ? "warn: " : "info: ", stdout);
    std::fputs(line.text, stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

void vlog(int level, const char* fmt, va_list ap) {
    char buf[kLen];
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    if (level) {
        g_warnings++;
    }
    push(level, buf);
}

}  // namespace

void sim_log_info(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(0, fmt, ap);
    va_end(ap);
}

void sim_log_warn(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(1, fmt, ap);
    va_end(ap);
}

void sim_info_once(const char* key, const char* fmt, ...) {
    if (!g_once.insert(std::string("i:") + key).second) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    vlog(0, fmt, ap);
    va_end(ap);
}

void sim_warn_once(const char* key, const char* fmt, ...) {
    if (!g_once.insert(std::string("w:") + key).second) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    vlog(1, fmt, ap);
    va_end(ap);
}

void sim_log_clear() {
    g_next = 0;
    g_filled = 0;
    g_warnings = 0;
    g_once.clear();
}

int sim_warning_count() {
    return g_warnings;
}

int sim_log_count() {
    return g_filled;
}

const char* sim_log_line(int oldest_index) {
    if (oldest_index < 0 || oldest_index >= g_filled) {
        return "";
    }
    int start = (g_next - g_filled + kCap) % kCap;
    return g_lines[(start + oldest_index) % kCap].text;
}

int sim_log_level(int oldest_index) {
    if (oldest_index < 0 || oldest_index >= g_filled) {
        return 0;
    }
    int start = (g_next - g_filled + kCap) % kCap;
    return g_lines[(start + oldest_index) % kCap].level;
}
