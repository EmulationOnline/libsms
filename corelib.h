#pragma once

#include <stdint.h>
#include <stddef.h>


enum Keys {
    BTN_A = 0,
    BTN_B,
    BTN_unused1,
    BTN_unused2,
    BTN_Up,
    BTN_Down,
    BTN_Left,
    BTN_Right,
    NUM_KEYS
};

#define VIDEO_WIDTH 320  // Both include overscan padding
#define VIDEO_HEIGHT 288

#define UNMANGLE extern "C"
#define EXPOSE extern "C" __attribute__((visibility("default")))

// Used by core to log to ui. Frontends are expected to define this.
UNMANGLE void corelib_set_puts(void(*cb)(const char*));

UNMANGLE void set_key(size_t key, char val);
UNMANGLE void init(const uint8_t* data, size_t len);
UNMANGLE const uint8_t *framebuffer();
UNMANGLE void frame();
UNMANGLE void dump_state(const char* save_path);
UNMANGLE void load_state(const char* save_path);
// Interface used by app. App closes fd.
UNMANGLE void save(int fd);
UNMANGLE void load(int fd);

// APU
const int SAMPLE_RATE = 44100;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / 60;
UNMANGLE long apu_sample_variable(int16_t *output, int32_t frames);
