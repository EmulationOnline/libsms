#include "corelib.h"
#include "Gearsystem/src/GearsystemCore.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
const int DISPLAY_WIDTH = 160;
const int DISPLAY_HEIGHT = 144;
uint32_t fbuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

void log(const char* msg) {
#ifdef DBG
    printf("%s\n", msg);
#endif
}

EXPOSE
void zero() {
    printf("zeroing.\n");
    sms_.Init();
}

EXPOSE
void set_key(size_t key, char val) {}

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
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
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

EXPOSE
long apu_sample_variable(int16_t* output, int32_t samples) {
    REQUIRE_CORE(0);
    return 0;
}

EXPOSE
void frame() {
    REQUIRE_CORE();
}

#ifndef __wasm32__
// save&load unsupported for wasm

EXPOSE
void save(int fd) {
    REQUIRE_CORE();
    // size_t state_size = gameboy_->stateSize();
    // printf("save state is %zu bytes\n", state_size);
    // uint8_t *buffer = (uint8_t*)malloc(state_size);
    // uint8_t* const orig_buffer = buffer;
    // gameboy_->saveState(buffer);
    // while (state_size > 0) {
    //     ssize_t written = write(fd, buffer, state_size);
    //     if (written <= 0) {
    //         perror("Save failed: ");
    //         return;
    //     }
    //     state_size -= written;
    //     buffer += written;
    // }
    // free(orig_buffer);
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
//    ssize_t bytes = lseek(fd, 0, SEEK_END);
//    if (bytes <= 0) {
//        perror("Failed to seek while loading: ");
//        return;
//    }
//    printf("Loading %zu bytes\n", bytes);
//    size_t state_size = gameboy_->stateSize();
//    if (bytes != state_size) {
//        puts("Invalid state size");
//        return;
//    }
//    lseek(fd, 0, SEEK_SET);
//    uint8_t *buffer = (uint8_t*)malloc(bytes);
//    uint8_t *write = buffer;
//    while (bytes > 0) {
//        ssize_t read_bytes = read(fd, write, bytes);
//        if (read_bytes <= 0) {
//            perror("Read failure during load: ");
//            return;
//        }
//        printf("read returned %zu bytes\n", read_bytes);
//        write += read_bytes;
//        bytes -= read_bytes;
//    }
//    gameboy_->loadState(buffer);
//    free(buffer);
}

EXPOSE
void load_state(const char* filename) {
    REQUIRE_CORE();
//    int fd = open(filename,  O_RDONLY , 0700);
//    if (fd == -1) {
//        perror("Failed to open: ");
//        return;
//    }
//    load(fd);
}

#endif // ifndef __wasm32__
