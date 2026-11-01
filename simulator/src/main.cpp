#include "cst816.h"
#include "gpio.h"
#include "host.h"
#include "loader.h"
#include "sim_api.h"
#include "st7789.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

const char* kBanner =
    "pixxcraft simulator\n"
    "Seeed XIAO ESP32-S3 + Waveshare 1.69\" touch LCD (ST7789V2, CST816)\n"
    "Glass is 240x280 and shows ST7789 GRAM rows 20-299. SPI mode 0, RGB565, no MISO.\n"
    "BL GPIO1  CS GPIO2  DC GPIO3  RST GPIO4\n"
    "TP SDA GPIO5  SCL GPIO6  INT GPIO43  RST GPIO44\n"
    "LCD SCK GPIO7  MOSI GPIO9    SPI.begin() uses those. transfer() does not toggle CS.\n"
    "Wire.begin() uses SDA 5 / SCL 6. Touch address is 7-bit 0x15.\n"
    "Load code with the button, or drop .ino and .cpp files on the window.\n"
    "Click the panel for a finger. Esc closes the window. Stop unloads the sketch.\n";

void usage() {
    std::fputs(
        "pixxcraft_sim [--scale N] [--load file.ino] [--frames N] [--dump file.bmp]\n"
        "              [--self-test] [--headless]\n"
        "With no --load, the window stays open until you load a sketch.\n",
        stdout);
}

}  // namespace

int main(int argc, char** argv) {
    bool self_test = false;
    bool headless = false;
    const char* dump = nullptr;
    int scale = 3;
    int frame_limit = 0;
    std::vector<std::string> loads;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--self-test") == 0) {
            self_test = true;
        } else if (std::strcmp(argv[i], "--headless") == 0) {
            headless = true;
        } else if (std::strcmp(argv[i], "--dump") == 0 && i + 1 < argc) {
            dump = argv[++i];
        } else if (std::strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            scale = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frame_limit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            loads.emplace_back(argv[++i]);
        } else if (std::strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else {
            std::fprintf(stderr, "unknown argument %s\n", argv[i]);
            usage();
            return 2;
        }
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::fputs(kBanner, stdout);

    gpio_init();
    st7789_init();
    cst816_init();
    if (!loader_init()) {
        return 1;
    }

    if (!headless && !host_init(scale)) {
        if (!self_test && loads.empty()) {
            return 1;
        }
        std::fputs("SDL window unavailable; continuing without it.\n", stderr);
        headless = true;
    }

    int code = 0;
    if (self_test) {
        host_enable_ui(false);
        code = run_self_test(dump);
        host_delay(200);
        host_shutdown();
        return code;
    }

    if (!loads.empty()) {
        loader_queue_narrow(loads);
        loader_compile_pending();
        if (!loader_loaded()) {
            host_shutdown();
            return 1;
        }
    }

    volatile int frames_done = 0;
    while (!host_quit()) {
        if (frame_limit > 0 && frames_done >= frame_limit) {
            break;
        }
        host_pump();
        if (loader_has_pending() && !loader_armed()) {
            loader_compile_pending();
        }
        if (!loader_loaded()) {
            if (!host_ready()) {
                break;
            }
            host_delay(16);
            continue;
        }
        jmp_buf env;
        if (setjmp(env) == 0) {
            loader_arm(&env);
            uint32_t started = host_millis();
            loader_run_once();
            loader_disarm();
            frames_done++;
            uint32_t elapsed = host_millis() - started;
            if (elapsed < 8 && host_ready()) {
                host_delay(8 - elapsed);
            }
        } else {
            loader_disarm();
            loader_on_abort();
        }
    }

    if (dump != nullptr) {
        sim_save_bmp(dump);
    }
    host_shutdown();
    return code;
}
