# force rebuild, while deps isn't working
.PHONY: default libsms.so clean run runc all ci deps deps-main wrepl

default: libsms.so

all: libsms.so main

deps:
	apt install -y g++
debs-main: deps
	apt install -y libsdl2-dev

ci: deps libsms.so

EMBEDFLAGS=-O3 -fvisibility=hidden -static-libstdc++ -fPIC
# CFLAGS=-fvisibility=hidden -ffreestanding -nostdlib -fPIC -O3 -Wfatal-errors -Werror
SRCS := $(wildcard Gearsystem/src/**/*.cpp Gearsystem/src/*.cpp Gearsystem/platforms/libretro/*.cpp Gearsystem/src/audio/emu2413/*.c Gearsystem/src/miniz/*.c)
SMSFLAGS=-Wfatal-errors -Werror -Wno-narrowing -D__LIBRETRO__ -I Gearsystem/src -I Gearsystem/src/audio -I Gearsystem/src/miniz/ -I Gearsystem/platforms/libretro -I Gearsystem/src/audio/emu2413 -Wno-div-by-zero
libsms.so: libsms.cpp corelib.h
	$(CXX) $(CFLAGS) $(EMBEDFLAGS) $(SMSFLAGS) -shared -o libsms.so libsms.cpp $(SRCS)
	cp libsms.so libapu.so
	echo "libsms done"

main: main.c corelib.h
	$(CXX) -O3 -o main main.c -L. -l:libsms.so -lSDL2 -lc -lm ${WARN}
	echo "main done"

clean:
	rm -f libsms.so

gdb:
	LD_LIBRARY_PATH=$(shell pwd) gdb --args ./main "$(ROM)"
run:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)"
runc:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)" c

repl:
	ls libsms.cpp Makefile | entr -c make all
wrepl:
	ls libsms.cpp Makefile | entr -c make libsms.js
	
# EMCC=~/external/emsdk/upstream/bin/wasm32-clang++
EMCC=~/external/emscripten/em++
EXPORTS="['_framebuffer_bytes', '_frame', '_init', '_alloc_rom']"
WASMFLAGS=-Wl,--no-entry -Wl,--export-all -s EXPORTED_FUNCTIONS=$(EXPORTS) -s EXPORTED_RUNTIME_METHODS=['HEAPU8'] -DISWASM

.PHONY: libsms.js
libsms.js:
	 $(EMCC) $(CFLAGS) $(SMSFLAGS) $(WASMFLAGS) -o libsms.js libsms.cpp $(SRCS) 
