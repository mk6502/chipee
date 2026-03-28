CC=cc
CFLAGS=-Wall
LDFLAGS=-lm

# Use pkg-config for SDL2 if available (handles both Intel and Apple Silicon Macs,
# as well as Linux). Falls back to brew prefix detection, then /usr/local.
ifeq ($(shell pkg-config --exists sdl2 2>/dev/null && echo yes),yes)
    CFLAGS  += $(shell pkg-config --cflags sdl2)
    LDFLAGS += $(shell pkg-config --libs sdl2)
else ifneq ($(shell command -v brew 2>/dev/null),)
    SDL2_PREFIX := $(shell brew --prefix sdl2 2>/dev/null)
    CFLAGS  += -I$(SDL2_PREFIX)/include
    LDFLAGS += -L$(SDL2_PREFIX)/lib -lSDL2
else
    CFLAGS  += -I/usr/local/include
    LDFLAGS += -lSDL2 -L/usr/local/lib
endif

all:
	$(CC) -o chipee sound.c emulator.c display.c chipee.c $(CFLAGS) $(LDFLAGS)

test:
	$(CC) -o chipee_tests chipee_tests.c chipee.c $(CFLAGS) $(LDFLAGS)
	./chipee_tests

clean:
	rm -f chipee chipee_tests
