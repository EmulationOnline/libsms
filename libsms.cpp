#include "corelib.h"
#include "ring.hpp"
#include "Gearsystem/src/GearsystemCore.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#define DBG true

#ifndef DBG
#define DBG false
#endif

#if DBG
#include <stdio.h>
#else
void printf(const char* msg, ...) {}
#endif

#define REQUIRE_CORE(val) if (!has_init_) { printf("no core. skipping %s\n", __func__); return val; }

GearsystemCore sms_;
bool has_init_ = false;
uint32_t megabuffer[OVER_WIDTH * OVER_HEIGHT];  // core needs to dump w/ overscan somewhere
uint32_t fbuffer[VIDEO_WIDTH * VIDEO_HEIGHT];   // non-overscan buffer.
int16_t abuffer[SAMPLE_RATE];  // buffer at most 1sec audio
Ring<int16_t, SAMPLE_RATE> ring_;

void (*corelib_puts)(const char* msg);

EXPOSE
void corelib_set_puts(void(*cb)(const char*)) {
    corelib_puts = cb;
    corelib_puts("corelib_puts initialized");
}
#ifndef ISWASM
#define puts(msg) corelib_puts(msg);
#endif

void log(const char* msg) {
#ifdef DBG
    puts(msg);
#endif
}

EXPOSE
void zero() {
    printf("zeroing.\n");
    sms_.Init();
}

EXPOSE
void set_key(size_t key, char val) {
    GS_Keys gsk;
    switch(key) {
        case BTN_A: gsk = Key_2; break;
        case BTN_B: gsk = Key_1; break;
        case BTN_Up: gsk = Key_Up; break;
        case BTN_Down: gsk = Key_Down; break;
        case BTN_Left: gsk = Key_Left; break;
        case BTN_Right: gsk = Key_Right; break;
        default: return;
    }

    constexpr GS_Joypads joypad = Joypad_1;
    if (val) {
        sms_.KeyPressed(joypad, gsk);
    } else {
        sms_.KeyReleased(joypad, gsk);
    }
}

uint8_t rom_buffer_[10*1024*1024];
// frontend needs a wasm buffer into which it can copy the rom.
EXPOSE
uint8_t *alloc_rom(size_t bytes) {
    if (bytes > sizeof(rom_buffer_)) {
        printf("rom too large, failed to load\n");
         return 0;
    }
    return rom_buffer_;
}

EXPOSE
void init(const uint8_t* data, size_t len) {
    printf("libsms init\n");

    sms_.Init();
    if (!sms_.LoadROMFromBuffer(data, len)) {
        puts("ROM load failed.");
        return;
    }
    has_init_ = true;
}

EXPOSE
const uint8_t *framebuffer() {
#ifdef ISWASM
    // ensure all pixels have 255 alpha
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
        fbuffer[i] |= 0xff000000;
    }
#endif
    return (const uint8_t*)fbuffer;
}

extern "C" 
__attribute__((visibility("default")))
size_t framebuffer_bytes() {
    return sizeof fbuffer;
}

size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

int16_t last_sample_ = 0;
EXPOSE
long apu_sample_variable(int16_t* output, int32_t samples) {
    REQUIRE_CORE(0);
    long count = ring_.pull(output, samples);
    if (count > 0) {
        last_sample_ = output[count - 1];
    }
    // if (read < samples) {
    //     printf("underflow: want=%u, had=%zu\n", samples, read);
    // }
    for (int i = count; count < samples; count++) {
        output[i] = last_sample_;
    }
    return count;
}

EXPOSE
void frame() {
    REQUIRE_CORE();
    int samples = 0;
    sms_.RunToVBlank((uint8_t*)&megabuffer, abuffer, &samples);
    samples /= 2;
    // Core produces stereo, convert to mono
    for (int i = 0; i < samples; i++) {
        abuffer[i] = abuffer[2*i];
    }
    int pushed = ring_.push(abuffer, samples);
    if (pushed < samples) {
        printf("ring overflow: %d / %d pushed\n", pushed, samples);
    }
    auto *video = sms_.GetVideo();
    video->Render32bit(video->GetFrameBuffer(), (uint8_t*)fbuffer, GS_PIXEL_RGBA8888, VIDEO_WIDTH*VIDEO_HEIGHT, /*overscan*/false);
}


EXPOSE
__attribute__((visibility("default")))
int save_str(uint8_t* dest, int capacity) {
    // Returns bytes saved, and writes to dest. 
    // Dest may be null to calculate size only. returns < 0 on error.
    REQUIRE_CORE(-1);
    const bool estimate = dest == NULL;

    if (estimate) {
        // size is unknown apriori, give it a huge allocation
        size_t sz = 100'000'000;
        dest = (uint8_t*)malloc(sz);
        capacity = sz;
    }

    size_t bytes = 0;
    sms_.SaveState(dest, bytes);
    printf("Wrote %zu bytes\n", bytes);
    assert(bytes <= capacity);

    if (estimate) {
        free(dest);
    }
    return bytes;
}

EXPOSE
__attribute__((visibility("default")))
void load_str(int len, const uint8_t *src) {
    REQUIRE_CORE();
}

#ifndef __wasm32__
// save&load unsupported for wasm

constexpr size_t MAX_STATE_SIZE = 100'000'000; // 100M
EXPOSE
void save(int fd) {
    REQUIRE_CORE();
    size_t bytes = save_str(NULL, 0);
    uint8_t *buffer = (uint8_t*)malloc(bytes);
    uint8_t* const orig_buffer = buffer;  // ptr to start
    save_str(buffer, bytes);
    while (bytes > 0) {
        ssize_t written = write(fd, buffer, bytes);
        if (written <= 0) {
            perror("Save failed: ");
            return;
        }
        bytes -= written;
        buffer += written;
    }
    free(orig_buffer);
}


EXPOSE
void dump_state(const char* filename) {
    REQUIRE_CORE();
    int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY , 0700);
    if (fd == -1) {
        perror("failed to open:");
        return;
    }
    printf("saving to %s\n", filename);
    save(fd);
}


EXPOSE
void load(int fd) {
    REQUIRE_CORE();
    ssize_t bytes = lseek(fd, 0, SEEK_END);
    if (bytes <= 0) {
        perror("Failed to seek while loading: ");
        return;
    }
    const size_t state_size = bytes;
    printf("Loading %zu bytes\n", bytes);
    lseek(fd, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t*)malloc(bytes);
    uint8_t *write = buffer;
    while (bytes > 0) {
        ssize_t read_bytes = read(fd, write, bytes);
        if (read_bytes <= 0) {
            perror("Read failure during load: ");
            return;
        }
        printf("read returned %zu bytes\n", read_bytes);
        write += read_bytes;
        bytes -= read_bytes;
    }
    sms_.LoadState(buffer, state_size);
    free(buffer);
}

EXPOSE
void load_state(const char* filename) {
    REQUIRE_CORE();
    int fd = open(filename,  O_RDONLY , 0700);
    if (fd == -1) {
        perror("Failed to open: ");
        return;
    }
    load(fd);
}

#endif // ifndef __wasm32__
