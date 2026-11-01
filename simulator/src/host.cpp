#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <cderr.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include "board.h"
#include "cst816.h"
#include "font8x8_basic.h"
#include "gpio.h"
#include "host.h"
#include "loader.h"
#include "log.h"
#include "sim_api.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

bool g_ready = false;
bool g_quit = false;
int g_scale = 3;
int g_pump_depth = 0;
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
SDL_Texture* g_texture = nullptr;
uint32_t g_last_present = 0;
LARGE_INTEGER g_freq = {};
LARGE_INTEGER g_t0 = {};
bool g_time = false;

constexpr int kMargin = 16;
constexpr int kSide = 448;
constexpr int kPad = 20;
constexpr int kNameH = 28;
constexpr int kUsbH = 34;
bool g_ui = true;

struct UiLayout {
    int glass_x;
    int glass_y;
    int board_x;
    int board_y;
    int board_w;
    int board_h;
    int side_x;
    SDL_Rect load_btn;
    SDL_Rect stop_btn;
};

UiLayout ui_layout() {
    UiLayout u{};
    int gw = kGlassW * g_scale;
    int gh = kGlassH * g_scale;
    u.board_x = kMargin;
    u.board_y = kMargin;
    u.glass_x = u.board_x + kPad;
    u.glass_y = u.board_y + kNameH + kPad;
    u.board_w = kPad + gw + kPad;
    u.board_h = kNameH + kPad + gh + kPad + kUsbH;
    u.side_x = u.board_x + u.board_w + kMargin;
    u.load_btn = SDL_Rect{u.side_x, kMargin, 132, 28};
    u.stop_btn = SDL_Rect{u.side_x + 140, kMargin, 72, 28};
    return u;
}

bool in_rect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

bool pointer_xy(int* x, int* y) {
    if (g_window == nullptr || g_renderer == nullptr) {
        return false;
    }
    int wx = 1;
    int wy = 1;
    int rx = 1;
    int ry = 1;
    SDL_GetWindowSize(g_window, &wx, &wy);
    SDL_GetRendererOutputSize(g_renderer, &rx, &ry);
    SDL_GetMouseState(x, y);
    if (wx > 0 && wy > 0) {
        *x = *x * rx / wx;
        *y = *y * ry / wy;
    }
    return true;
}

void draw_text(int x, int y, const char* text, uint8_t r, uint8_t g, uint8_t b);

void draw_button(const SDL_Rect& rect, const char* label, bool gold) {
    SDL_SetRenderDrawColor(g_renderer, gold ? 214 : 78, gold ? 188 : 70, gold ? 110 : 64, 255);
    SDL_RenderFillRect(g_renderer, &rect);
    SDL_SetRenderDrawColor(g_renderer, 28, 26, 22, 255);
    SDL_RenderDrawRect(g_renderer, &rect);
    int width = (int)std::strlen(label) * 8;
    draw_text(rect.x + (rect.w - width) / 2, rect.y + 10, label, gold ? 32 : 220, gold ? 26 : 206, gold ? 14 : 190);
}

std::wstring widen_utf8(const char* text) {
    if (text == nullptr || text[0] == 0) {
        return L"";
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (n <= 0) {
        return L"";
    }
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, out.data(), n);
    if (!out.empty() && out.back() == L'\0') {
        out.pop_back();
    }
    return out;
}

std::vector<std::wstring> parse_open_files(const wchar_t* buf) {
    std::vector<std::wstring> out;
    const wchar_t* cursor = buf;
    std::wstring first = cursor;
    cursor += first.size() + 1;
    if (*cursor == 0) {
        if (!first.empty()) {
            out.push_back(first);
        }
        return out;
    }
    while (*cursor != 0) {
        std::wstring name = cursor;
        if (!name.empty()) {
            out.push_back(first + L"\\" + name);
        }
        cursor += name.size() + 1;
    }
    return out;
}

std::vector<std::wstring> open_code_dialog() {
    wchar_t buffer[8192];
    buffer[0] = 0;
    std::wstring start = loader_start_dir();
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof ofn;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = 8192;
    ofn.lpstrFilter = L"Sketch code\0*.ino;*.cpp;*.cc;*.cxx;*.c;*.h;*.hpp\0All files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Load sketch code";
    ofn.lpstrInitialDir = start.c_str();
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY |
                OFN_NOCHANGEDIR;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (g_window != nullptr && SDL_GetWindowWMInfo(g_window, &info) == SDL_TRUE && info.subsystem == SDL_SYSWM_WINDOWS) {
        ofn.hwndOwner = info.info.win.window;
    }
    if (!GetOpenFileNameW(&ofn)) {
        if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {
            sim_log_warn("too many files selected");
        }
        return {};
    }
    return parse_open_files(buffer);
}

void ensure_time() {
    if (g_time) {
        return;
    }
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_t0);
    g_time = true;
}

uint64_t now_us() {
    ensure_time();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart - g_t0.QuadPart) * 1000000ull / (uint64_t)g_freq.QuadPart);
}

void draw_text(int x, int y, const char* text, uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(g_renderer, r, g, b, 255);
    int cx = x;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != 0; ++p) {
        if (*p == '\n') {
            cx = x;
            y += 10;
            continue;
        }
        unsigned char ch = *p < 32 || *p > 127 ? '?' : *p;
        const unsigned char* glyph = font8x8_basic[ch];
        for (int row = 0; row < 8; row++) {
            unsigned bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if ((bits & (1u << col)) != 0) {
                    SDL_RenderDrawPoint(g_renderer, cx + col, y + row);
                }
            }
        }
        cx += 8;
    }
}

int draw_wrapped(int x, int y, int max_chars, const char* text, uint8_t r, uint8_t g, uint8_t b) {
    char line[80];
    int n = 0;
    if (max_chars > 78) {
        max_chars = 78;
    }
    for (const char* p = text;; ++p) {
        bool end = *p == 0 || *p == '\n';
        if (end || n >= max_chars) {
            if (n == max_chars) {
                int break_at = n;
                while (break_at > 12 && line[break_at - 1] != ' ') {
                    break_at--;
                }
                if (break_at > 12 && *p != 0) {
                    line[break_at] = 0;
                    draw_text(x, y, line, r, g, b);
                    y += 10;
                    int keep = n - break_at;
                    std::memmove(line, line + break_at, (size_t)keep);
                    n = keep;
                    if (!end) {
                        line[n++] = *p;
                        continue;
                    }
                }
            }
            line[n] = 0;
            if (n > 0) {
                draw_text(x, y, line, r, g, b);
                y += 10;
            }
            n = 0;
            if (*p == 0) {
                break;
            }
            continue;
        }
        line[n++] = *p;
    }
    return y;
}

void paint() {
    if (g_renderer == nullptr || g_texture == nullptr) {
        return;
    }
    static uint8_t frame[kGlassW * kGlassH * 3];
    for (int y = 0; y < kGlassH; y++) {
        for (int x = 0; x < kGlassW; x++) {
            uint32_t color = sim_glass_rgb(x, y);
            int i = (y * kGlassW + x) * 3;
            frame[i + 0] = (uint8_t)(color >> 16);
            frame[i + 1] = (uint8_t)(color >> 8);
            frame[i + 2] = (uint8_t)color;
        }
    }
    SDL_UpdateTexture(g_texture, nullptr, frame, kGlassW * 3);

    SimStatus status;
    sim_collect_status(&status);
    UiLayout u = ui_layout();

    SDL_SetRenderDrawColor(g_renderer, 24, 26, 28, 255);
    SDL_RenderClear(g_renderer);

    SDL_Rect board{u.board_x, u.board_y, u.board_w, u.board_h};
    SDL_SetRenderDrawColor(g_renderer, 14, 46, 38, 255);
    SDL_RenderFillRect(g_renderer, &board);
    SDL_SetRenderDrawColor(g_renderer, 40, 78, 64, 255);
    SDL_RenderDrawRect(g_renderer, &board);
    draw_text(u.board_x + 16, u.board_y + 10, "XIAO ESP32-S3", 176, 196, 168);
    SDL_Rect usb{u.board_x + (u.board_w - 78) / 2, u.board_y + u.board_h - 26, 78, 16};
    SDL_SetRenderDrawColor(g_renderer, 168, 172, 176, 255);
    SDL_RenderFillRect(g_renderer, &usb);

    SDL_Rect dst{u.glass_x, u.glass_y, kGlassW * g_scale, kGlassH * g_scale};
    bool live = status.blank[0] == 0;
    SDL_SetRenderDrawColor(g_renderer, live ? 70 : 210, live ? 170 : 140, live ? 90 : 50, 255);
    SDL_Rect border{dst.x - 3, dst.y - 3, dst.w + 6, dst.h + 6};
    SDL_RenderFillRect(g_renderer, &border);
    SDL_RenderCopy(g_renderer, g_texture, nullptr, &dst);

    int x = u.side_x;
    int y = kMargin;
    draw_button(u.load_btn, "Load code", true);
    draw_button(u.stop_btn, "Stop", false);
    y = u.load_btn.y + u.load_btn.h + 12;
    y = draw_wrapped(x, y, 54, loader_status(), 232, 214, 150);
    y += 4;
    draw_text(x, y, "drop .ino or .cpp files here", 150, 158, 148);
    y += 16;
    draw_text(x, y, "pixxcraft  240x280  ST7789", 220, 220, 210);
    y += 14;
    char line[96];
    std::snprintf(line, sizeof line, "BL %d  RST %s  SLP %s  DISP %s  INV %s",
                  status.backlight,
                  status.reset_held ? "held" : "ok",
                  status.sleep ? "in" : "out",
                  status.display_on ? "on" : "off",
                  status.inverted ? "on" : "off");
    draw_text(x, y, line, 190, 190, 180);
    y += 12;
    std::snprintf(line, sizeof line, "MADCTL %02X  COLMOD %02X  CS %s  DC %s",
                  status.madctl,
                  status.colmod,
                  status.cs_selected ? "low" : "high",
                  status.dc_data ? "data" : "cmd");
    draw_text(x, y, line, 190, 190, 180);
    y += 12;
    std::snprintf(line, sizeof line, "window x %d-%d  y %d-%d", status.xs, status.xe, status.ys, status.ye);
    draw_text(x, y, line, 190, 190, 180);
    y += 12;
    std::snprintf(line, sizeof line, "glass shows GRAM rows 20-299");
    draw_text(x, y, line, 160, 180, 150);
    y += 12;
    std::snprintf(line, sizeof line, "writes on glass %u   off glass %u", status.pixels_in, status.pixels_out);
    draw_text(x, y, line, 190, 190, 180);
    y += 12;
    std::snprintf(line, sizeof line, "last %s", status.last_cmd);
    draw_text(x, y, line, 190, 190, 180);
    y += 16;
    std::snprintf(line, sizeof line, "SPI %s  SCK %d MOSI %d mode %d",
                  status.spi_on_lcd ? "on LCD pins" : "NOT on LCD",
                  status.spi_sck,
                  status.spi_mosi,
                  status.spi_mode);
    draw_text(x, y, line, status.spi_on_lcd ? 170 : 230, status.spi_on_lcd ? 200 : 160, 120);
    y += 12;
    std::snprintf(line, sizeof line, "I2C %s  SDA %d SCL %d",
                  status.i2c_on_tp ? "on touch pins" : "not started",
                  status.i2c_sda,
                  status.i2c_scl);
    draw_text(x, y, line, status.i2c_on_tp ? 170 : 180, status.i2c_on_tp ? 200 : 180, 140);
    y += 12;
    if (status.touch_down) {
        std::snprintf(line, sizeof line, "finger %d,%d  INT low", status.touch_x, status.touch_y);
        draw_text(x, y, line, 120, 210, 255);
    } else {
        draw_text(x, y, "finger up  INT high", 160, 160, 160);
    }
    y += 12;
    std::snprintf(line, sizeof line, "LED_BUILTIN GPIO21 %s", status.led_on ? "on" : "off");
    draw_text(x, y, line, 160, 160, 160);
    y += 16;
    if (status.blank[0] == 0) {
        draw_text(x, y, "glass is live", 120, 200, 120);
        y += 12;
    } else {
        y = draw_wrapped(x, y, 54, status.blank, 230, 170, 90);
        y += 4;
    }
    y += 8;
    draw_text(x, y, "log", 140, 140, 140);
    y += 12;
    int count = sim_log_count();
    int first = count > 18 ? count - 18 : 0;
    for (int i = first; i < count; i++) {
        bool warn = sim_log_level(i) != 0;
        y = draw_wrapped(x, y, 54, sim_log_line(i), warn ? 230 : 170, warn ? 150 : 170, warn ? 80 : 160);
        if (y > u.board_y + u.board_h - 8) {
            break;
        }
    }
    SDL_RenderPresent(g_renderer);
    g_last_present = host_millis();
}

void apply_pointer() {
    int mx = 0;
    int my = 0;
    if (!pointer_xy(&mx, &my)) {
        return;
    }
    int raw_x = 0;
    int raw_y = 0;
    uint32_t buttons = SDL_GetMouseState(&raw_x, &raw_y);
    UiLayout u = ui_layout();
    int gx = (mx - u.glass_x) / g_scale;
    int gy = (my - u.glass_y) / g_scale;
    bool inside = gx >= 0 && gy >= 0 && gx < kGlassW && gy < kGlassH;
    bool down = inside && (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    if (!inside) {
        gx = 0;
        gy = 0;
    }
    cst816_apply_mouse(gx, gy, down);
}

void take_ui_action(bool want_load, bool want_stop, const std::vector<std::wstring>& dropped) {
    if (!g_ui || loader_busy()) {
        return;
    }
    if (want_stop) {
        loader_request_stop();
        return;
    }
    if (want_load) {
        std::vector<std::wstring> files = open_code_dialog();
        if (!files.empty()) {
            loader_queue(files);
        }
        return;
    }
    if (!dropped.empty()) {
        loader_queue(dropped);
    }
}

}  // namespace

uint32_t host_millis() {
    return (uint32_t)(now_us() / 1000ull);
}

uint32_t host_micros() {
    return (uint32_t)now_us();
}

bool host_ready() {
    return g_ready;
}

void host_set_title(const char* title) {
    if (g_window != nullptr && title != nullptr) {
        SDL_SetWindowTitle(g_window, title);
    }
}

void host_enable_ui(bool enabled) {
    g_ui = enabled;
}

bool host_quit() {
    return g_quit;
}

bool host_init(int scale) {
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 6) {
        scale = 6;
    }
    g_scale = scale;
    RECT work{};
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        int max_w = (int)(work.right - work.left) - 24;
        int max_h = (int)(work.bottom - work.top) - 48;
        while (g_scale > 1) {
            UiLayout u = ui_layout();
            int w = u.side_x + kSide + kMargin;
            int h = u.board_y + u.board_h + kMargin;
            if (w <= max_w && h <= max_h) {
                break;
            }
            g_scale--;
        }
    }
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "unaware");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    int w = 0;
    int h = 0;
    {
        UiLayout u = ui_layout();
        w = u.side_x + kSide + kMargin;
        h = u.board_y + u.board_h + kMargin;
    }
    g_window = SDL_CreateWindow(
        "pixxcraft  XIAO ESP32-S3 + 1.69 TFT",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_SHOWN);
    if (g_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (g_renderer == nullptr) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (g_renderer == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    g_texture = SDL_CreateTexture(
        g_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, kGlassW, kGlassH);
    if (g_texture == nullptr) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(g_texture, SDL_ScaleModeNearest);
    g_ready = true;
    g_quit = false;
    host_present();
    return true;
}

void host_shutdown() {
    if (g_texture != nullptr) {
        SDL_DestroyTexture(g_texture);
        g_texture = nullptr;
    }
    if (g_renderer != nullptr) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window != nullptr) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    if (g_ready) {
        SDL_Quit();
    }
    g_ready = false;
}

void host_present() {
    if (!g_ready) {
        return;
    }
    paint();
}

void host_pump() {
    if (!g_ready || g_pump_depth > 0) {
        return;
    }
    g_pump_depth++;
    bool want_load = false;
    bool want_stop = false;
    std::vector<std::wstring> dropped;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            g_quit = true;
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            g_quit = true;
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && g_ui) {
            int mx = 0;
            int my = 0;
            if (pointer_xy(&mx, &my)) {
                UiLayout u = ui_layout();
                if (in_rect(mx, my, u.load_btn)) {
                    want_load = true;
                } else if (in_rect(mx, my, u.stop_btn)) {
                    want_stop = true;
                }
            }
        } else if (event.type == SDL_DROPFILE && event.drop.file != nullptr) {
            dropped.push_back(widen_utf8(event.drop.file));
            SDL_free(event.drop.file);
        }
    }
    apply_pointer();
    gpio_fire_isrs();
    if (host_millis() - g_last_present >= 16) {
        paint();
    }
    g_pump_depth--;
    take_ui_action(want_load, want_stop, dropped);
}

void host_delay(uint32_t ms) {
    uint32_t start = host_millis();
    while ((host_millis() - start) < ms) {
        if (g_quit) {
            return;
        }
        if (g_ready) {
            host_pump();
        }
        Sleep(1);
    }
}

bool host_capture(const char* path) {
    if (!g_ready || g_renderer == nullptr || path == nullptr) {
        return false;
    }
    host_present();
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(g_renderer, &w, &h);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_RGB24);
    if (surface == nullptr) {
        return false;
    }
    if (SDL_RenderReadPixels(g_renderer, nullptr, SDL_PIXELFORMAT_RGB24, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        return false;
    }
    int rc = SDL_SaveBMP(surface, path);
    SDL_FreeSurface(surface);
    return rc == 0;
}
