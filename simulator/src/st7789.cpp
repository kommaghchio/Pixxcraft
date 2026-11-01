#include "st7789.h"

#include "board.h"
#include "gpio.h"
#include "log.h"

#include <cstdio>
#include <cstring>

namespace {

uint16_t g_gram[kGramW * kGramH];
int g_xs = 0;
int g_xe = 239;
int g_ys = 0;
int g_ye = 319;
int g_x = 0;
int g_y = 0;
bool g_ramwr = false;
uint8_t g_madctl = 0;
uint8_t g_colmod = 0x06;
bool g_sleep = true;
bool g_display_on = false;
bool g_inverted = false;
bool g_reset_held = false;
int g_pix_phase = 0;
uint8_t g_pix[3] = {};
uint8_t g_cmd = 0;
int g_arg_need = 0;
int g_arg_i = 0;
uint8_t g_args[4] = {};
unsigned g_pixels_in = 0;
unsigned g_pixels_out = 0;
char g_last_cmd[48] = "power-on";
bool g_cs_warn_armed = true;

bool mx() { return (g_madctl & 0x40) != 0; }
bool my() { return (g_madctl & 0x80) != 0; }
bool mv() { return (g_madctl & 0x20) != 0; }
bool bgr() { return (g_madctl & 0x08) != 0; }

int col_fmt() { return g_colmod & 0x07; }

bool reset_pin_held() {
    return gpio_is_output(kPinRst) && !gpio_read(kPinRst);
}

bool cs_selected() {
    return gpio_is_output(kPinCs) && !gpio_read(kPinCs);
}

void reset_regs() {
    g_sleep = true;
    g_display_on = false;
    g_inverted = false;
    g_madctl = 0;
    g_colmod = 0x06;
    g_xs = 0;
    g_xe = kGramW - 1;
    g_ys = 0;
    g_ye = kGramH - 1;
    g_ramwr = false;
    g_pix_phase = 0;
    g_arg_need = 0;
    g_arg_i = 0;
    std::snprintf(g_last_cmd, sizeof g_last_cmd, "reset");
}

void clear_gram() {
    std::memset(g_gram, 0, sizeof g_gram);
    g_pixels_in = 0;
    g_pixels_out = 0;
}

void note_cmd(uint8_t cmd, const char* name) {
    std::snprintf(g_last_cmd, sizeof g_last_cmd, "%s 0x%02X", name, cmd);
    char key[16];
    std::snprintf(key, sizeof key, "cmd-%02X", cmd);
    sim_info_once(key, "%s (0x%02X)", name, cmd);
}

void seek_start() {
    g_x = mx() ? g_xe : g_xs;
    g_y = my() ? g_ye : g_ys;
}

void step() {
    if (!mv()) {
        if (!mx()) {
            if (g_x < g_xe) {
                g_x++;
                return;
            }
            g_x = g_xs;
        } else {
            if (g_x > g_xs) {
                g_x--;
                return;
            }
            g_x = g_xe;
        }
        if (!my()) {
            if (g_y < g_ye) {
                g_y++;
            } else {
                g_y = g_ys;
            }
        } else if (g_y > g_ys) {
            g_y--;
        } else {
            g_y = g_ye;
        }
        return;
    }

    if (!my()) {
        if (g_y < g_ye) {
            g_y++;
            return;
        }
        g_y = g_ys;
    } else {
        if (g_y > g_ys) {
            g_y--;
            return;
        }
        g_y = g_ye;
    }
    if (!mx()) {
        if (g_x < g_xe) {
            g_x++;
        } else {
            g_x = g_xs;
        }
    } else if (g_x > g_xs) {
        g_x--;
    } else {
        g_x = g_xe;
    }
}

uint16_t swap_rb(uint16_t color) {
    return (uint16_t)((color & 0x07E0) | ((color & 0xF800) >> 11) | ((color & 0x001F) << 11));
}

void store_pixel(uint16_t color) {
    if (bgr()) {
        color = swap_rb(color);
    }
    bool in_gram = g_x >= 0 && g_x < kGramW && g_y >= 0 && g_y < kGramH;
    bool in_glass = in_gram && g_y >= kGlassY && g_y < kGlassY + kGlassH;
    if (in_gram) {
        g_gram[g_y * kGramW + g_x] = color;
    }
    if (in_glass) {
        g_pixels_in++;
    } else {
        g_pixels_out++;
        sim_warn_once(
            "offglass",
            "pixel (%d,%d) missed the glass. GRAM is %dx%d; this panel shows rows %d-%d. Use a Y offset of %d in RASET.",
            g_x,
            g_y,
            kGramW,
            kGramH,
            kGlassY,
            kGlassY + kGlassH - 1,
            kGlassY);
    }
    step();
}

void check_window_overlap() {
    int y0 = g_ys < g_ye ? g_ys : g_ye;
    int y1 = g_ys > g_ye ? g_ys : g_ye;
    int x0 = g_xs < g_xe ? g_xs : g_xe;
    int x1 = g_xs > g_xe ? g_xs : g_xe;
    bool overlaps = !(x1 < 0 || x0 >= kGramW || y1 < kGlassY || y0 >= kGlassY + kGlassH);
    if (!overlaps) {
        sim_warn_once(
            "window-miss",
            "address window x %d-%d y %d-%d does not overlap the glass (rows %d-%d)",
            g_xs,
            g_xe,
            g_ys,
            g_ye,
            kGlassY,
            kGlassY + kGlassH - 1);
    }
    if (g_xe >= kGramW || g_xs < 0 || g_xs > g_xe) {
        sim_warn_once("caset-range", "CASET x %d-%d is outside 0-%d", g_xs, g_xe, kGramW - 1);
    }
    if (g_ye >= kGramH || g_ys < 0 || g_ys > g_ye) {
        sim_warn_once("raset-range", "RASET y %d-%d is outside 0-%d", g_ys, g_ye, kGramH - 1);
    }
}

void note_ram_gates() {
    if (col_fmt() != 0x05) {
        sim_warn_once(
            "colmod",
            "COLMOD is 0x%02X. This panel path expects RGB565 (write 0x3A then 0x55 or 0x05) before pixel data.",
            g_colmod);
    } else if ((g_colmod & 0x70) != 0 && (g_colmod & 0x70) != 0x50) {
        sim_info_once("colmod565", "COLMOD 0x%02X accepted as 16-bit", g_colmod);
    }
    if (g_sleep) {
        sim_warn_once("ram-sleep", "pixel write while the controller is in sleep. Send SLPOUT 0x11 first.");
    }
    if (!g_display_on) {
        sim_warn_once("ram-dispoff", "pixel write while display output is off. Send DISPON 0x29.");
    }
    if (gpio_duty(kPinBl) == 0) {
        sim_warn_once("ram-bl", "pixel write while backlight GPIO1 is low. The glass will stay dark.");
    }
    if (g_reset_held) {
        sim_warn_once("ram-rst", "pixel write while LCD reset GPIO4 is held low.");
    }
}

void apply_args() {
    if (g_cmd == 0x2A) {
        g_xs = (g_args[0] << 8) | g_args[1];
        g_xe = (g_args[2] << 8) | g_args[3];
        std::snprintf(g_last_cmd, sizeof g_last_cmd, "CASET %d-%d", g_xs, g_xe);
    } else if (g_cmd == 0x2B) {
        g_ys = (g_args[0] << 8) | g_args[1];
        g_ye = (g_args[2] << 8) | g_args[3];
        std::snprintf(g_last_cmd, sizeof g_last_cmd, "RASET %d-%d", g_ys, g_ye);
    } else if (g_cmd == 0x36) {
        g_madctl = g_args[0];
        std::snprintf(g_last_cmd, sizeof g_last_cmd, "MADCTL 0x%02X", g_madctl);
    } else if (g_cmd == 0x3A) {
        g_colmod = g_args[0];
        std::snprintf(g_last_cmd, sizeof g_last_cmd, "COLMOD 0x%02X", g_colmod);
    } else if (g_cmd == 0x37) {
        int scroll = (g_args[0] << 8) | g_args[1];
        sim_warn_once(
            "vscrsadd",
            "VSCRSADD %d is not simulated. Put the Y offset of %d in RASET instead.",
            scroll,
            kGlassY);
    }
}

void on_command(uint8_t cmd) {
    if (g_arg_need > 0 && g_arg_i < g_arg_need) {
        sim_warn_once("short-cmd", "command 0x%02X got %d of %d data bytes", g_cmd, g_arg_i, g_arg_need);
    }
    if (g_pix_phase != 0) {
        sim_warn_once("partial-pixel", "pixel stream ended mid-pixel (%d byte%s waiting)", g_pix_phase, g_pix_phase == 1 ? "" : "s");
        g_pix_phase = 0;
    }
    g_cmd = cmd;
    g_arg_i = 0;
    g_ramwr = false;
    g_arg_need = 0;
    switch (cmd) {
    case 0x00:
        note_cmd(cmd, "NOP");
        break;
    case 0x01:
        note_cmd(cmd, "SWRESET");
        reset_regs();
        break;
    case 0x10:
        note_cmd(cmd, "SLPIN");
        g_sleep = true;
        break;
    case 0x11:
        note_cmd(cmd, "SLPOUT");
        g_sleep = false;
        break;
    case 0x13:
        note_cmd(cmd, "NORON");
        break;
    case 0x20:
        note_cmd(cmd, "INVOFF");
        g_inverted = false;
        break;
    case 0x21:
        note_cmd(cmd, "INVON");
        g_inverted = true;
        break;
    case 0x28:
        note_cmd(cmd, "DISPOFF");
        g_display_on = false;
        break;
    case 0x29:
        note_cmd(cmd, "DISPON");
        g_display_on = true;
        break;
    case 0x2A:
        note_cmd(cmd, "CASET");
        g_arg_need = 4;
        break;
    case 0x2B:
        note_cmd(cmd, "RASET");
        g_arg_need = 4;
        break;
    case 0x2C:
        note_cmd(cmd, "RAMWR");
        g_ramwr = true;
        g_pix_phase = 0;
        seek_start();
        check_window_overlap();
        note_ram_gates();
        break;
    case 0x36:
        note_cmd(cmd, "MADCTL");
        g_arg_need = 1;
        break;
    case 0x37:
        note_cmd(cmd, "VSCRSADD");
        g_arg_need = 2;
        break;
    case 0x3A:
        note_cmd(cmd, "COLMOD");
        g_arg_need = 1;
        break;
    default: {
        char name[24];
        std::snprintf(name, sizeof name, "cmd 0x%02X", cmd);
        note_cmd(cmd, name);
        g_arg_need = -1;
        break;
    }
    }
}

void push_pixel_byte(uint8_t data) {
    int fmt = col_fmt();
    if (fmt == 0x05) {
        if (g_pix_phase == 0) {
            g_pix[0] = data;
            g_pix_phase = 1;
            return;
        }
        store_pixel((uint16_t)((g_pix[0] << 8) | data));
        g_pix_phase = 0;
        return;
    }
    if (fmt == 0x06) {
        g_pix[g_pix_phase++] = data;
        if (g_pix_phase < 3) {
            return;
        }
        uint16_t color = (uint16_t)(((g_pix[0] >> 3) << 11) | ((g_pix[1] >> 2) << 5) | (g_pix[2] >> 3));
        store_pixel(color);
        g_pix_phase = 0;
        return;
    }
    sim_warn_once("colmod-bpp", "COLMOD 0x%02X pixel format is not 16-bit or 18-bit; bytes dropped", g_colmod);
}

void on_data(uint8_t data) {
    if (g_ramwr) {
        push_pixel_byte(data);
        return;
    }
    if (g_arg_need > 0 && g_arg_i < g_arg_need) {
        g_args[g_arg_i++] = data;
        if (g_arg_i == g_arg_need) {
            apply_args();
        }
        return;
    }
    if (g_arg_need < 0) {
        return;
    }
    sim_warn_once("extra-data", "data byte 0x%02X after command 0x%02X, which takes no more data", data, g_cmd);
}

bool gates_closed() {
    return g_reset_held || g_sleep || !g_display_on || gpio_duty(kPinBl) == 0;
}

}  // namespace

void st7789_init() {
    clear_gram();
    reset_regs();
    g_reset_held = false;
    g_cs_warn_armed = true;
    std::snprintf(g_last_cmd, sizeof g_last_cmd, "power-on");
}

void st7789_observe_reset() {
    bool held = reset_pin_held();
    if (held && !g_reset_held) {
        clear_gram();
        reset_regs();
        sim_info_once("rst-low", "LCD reset GPIO4 went low; GRAM cleared");
    } else if (!held && g_reset_held) {
        sim_info_once("rst-high", "LCD reset released. Controller is in sleep until SLPOUT 0x11 and DISPON 0x29.");
    }
    g_reset_held = held;
}

void st7789_spi_byte(uint8_t data) {
    st7789_observe_reset();
    if (!cs_selected()) {
        if (g_cs_warn_armed) {
            sim_log_warn("SPI byte ignored: LCD CS GPIO2 is high (active low). transfer() does not toggle it.");
            g_cs_warn_armed = false;
        }
        return;
    }
    g_cs_warn_armed = true;
    if (g_reset_held) {
        sim_warn_once("spi-in-reset", "SPI byte ignored: LCD reset GPIO4 is held low");
        return;
    }
    if (!gpio_is_output(kPinDc)) {
        sim_warn_once("dc-mode", "LCD D/C GPIO3 is not an output; bytes have no command/data select");
        return;
    }
    if (!gpio_read(kPinDc)) {
        on_command(data);
    } else {
        on_data(data);
    }
}

uint32_t st7789_glass_rgb(int x, int y) {
    st7789_observe_reset();
    if (x < 0 || y < 0 || x >= kGlassW || y >= kGlassH) {
        return 0;
    }
    if (g_reset_held || g_sleep || !g_display_on) {
        return 0;
    }
    int duty = gpio_duty(kPinBl);
    if (duty <= 0) {
        return 0;
    }
    int gy = y + kGlassY;
    // INVON is logged in the side panel, but this IPS treats it as polarity
    // correction. Glass shows GRAM as written (Adafruit black stays black).
    uint16_t color = g_gram[gy * kGramW + x];
    int r = ((color >> 11) & 31) * 255 / 31;
    int g = ((color >> 5) & 63) * 255 / 63;
    int b = (color & 31) * 255 / 31;
    r = r * duty / 255;
    g = g * duty / 255;
    b = b * duty / 255;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void st7789_fill_status(SimStatus* out) {
    st7789_observe_reset();
    out->backlight = gpio_duty(kPinBl);
    out->reset_held = g_reset_held;
    out->sleep = g_sleep;
    out->display_on = g_display_on;
    out->inverted = g_inverted;
    out->cs_selected = cs_selected();
    out->dc_data = gpio_is_output(kPinDc) && gpio_read(kPinDc);
    out->madctl = g_madctl;
    out->colmod = g_colmod;
    out->xs = g_xs;
    out->xe = g_xe;
    out->ys = g_ys;
    out->ye = g_ye;
    out->pixels_in = g_pixels_in;
    out->pixels_out = g_pixels_out;
    std::snprintf(out->last_cmd, sizeof out->last_cmd, "%s", g_last_cmd);
    out->blank[0] = 0;
    bool any = false;
    auto add = [&](const char* text) {
        if (any && std::strlen(out->blank) + 2 < sizeof out->blank) {
            std::strcat(out->blank, "; ");
        }
        any = true;
        std::snprintf(out->blank + std::strlen(out->blank), sizeof out->blank - std::strlen(out->blank), "%s", text);
    };
    if (g_reset_held) {
        add("LCD reset is held low on GPIO4");
    }
    if (out->backlight == 0) {
        add("backlight is off (drive GPIO1 high)");
    }
    if (g_sleep) {
        add("controller is in sleep (send 0x11 SLPOUT)");
    }
    if (!g_display_on) {
        add("display output is off (send 0x29 DISPON)");
    }
    if (!gates_closed() && g_pixels_in == 0 && g_pixels_out > 0) {
        add("pixels missed the glass (GRAM rows 20-299)");
    } else if (!gates_closed() && g_pixels_in == 0 && g_pixels_out == 0) {
        add("no pixels written yet");
    }
}
