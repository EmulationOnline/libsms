#pragma once

#include <stdint.h>
#include <stddef.h>
#include "Gearsystem/src/definitions.h"

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

#define VIDEO_WIDTH GS_RESOLUTION_MAX_WIDTH
#define VIDEO_HEIGHT GS_RESOLUTION_MAX_HEIGHT
#define OVER_WIDTH GS_RESOLUTION_MAX_WIDTH_WITH_OVERSCAN
#define OVER_HEIGHT GS_RESOLUTION_MAX_HEIGHT_WITH_OVERSCAN

#define UNMANGLE extern "C"
#define EXPOSE extern "C" __attribute__((visibility("default")))

// Used by core to log to ui. Frontends are expected to define this.
UNMANGLE void corelib_set_puts(void(*cb)(const char*));

UNMANGLE void set_key(size_t key, char val);
UNMANGLE void init(const uint8_t* data, size_t len);
UNMANGLE const uint8_t *framebuffer();
UNMANGLE void frame();
// dynamic video
UNMANGLE int framerate();
UNMANGLE int width();
UNMANGLE int height();

UNMANGLE void dump_state(const char* save_path);
UNMANGLE void load_state(const char* save_path);
// Interface used by app. App closes fd.
UNMANGLE void save(int fd);
UNMANGLE void load(int fd);
// Returns bytes saved, and writes to dest. 
// Dest may be null to calculate size only. returns < 0 on error.
UNMANGLE int save_str(uint8_t* dest, int capacity);
// Loads len bytes from src
UNMANGLE void load_str(int len, const uint8_t* src);

// APU
const int SAMPLE_RATE = 44100;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / 60;
UNMANGLE long apu_sample_variable(int16_t *output, int32_t frames);
