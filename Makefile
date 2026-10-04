CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra -Wno-unused-parameter -std=gnu11 $(shell sdl2-config --cflags)
LDLIBS  := $(shell sdl2-config --libs) -lm
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)

all: toptennis tools/assetview

toptennis: $(filter-out build/assetview.o,$(OBJ))
	$(CC) -o $@ $^ $(LDLIBS)

tools/assetview: build/assetview.o $(filter-out build/main.o build/assetview.o,$(OBJ))
	$(CC) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

clean:
	rm -rf build toptennis tools/assetview

test: tests/test_state
	python3 tests/test_original_data.py
	$(CC) -std=gnu11 -Wall -Wextra -Werror -o tests/test_ctrl tests/test_ctrl.c src/ctrl.c src/shot.c
	./tests/test_ctrl
	$(CC) -std=gnu11 -Wall -Wextra -Werror -o tests/test_score tests/test_score.c src/score.c
	./tests/test_score
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -Wl,--gc-sections -o tests/test_court tests/test_court.c src/game.c src/gfx.c src/video.c $(LDLIBS)
	./tests/test_court
	$(CC) $(CFLAGS) -D__vita__ -o tests/test_pointer tests/test_pointer.c src/platform.c $(LDLIBS)
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./tests/test_pointer
	./tests/test_state

tests/test_engine: tests/test_engine.c $(filter-out build/main.o build/assetview.o,$(OBJ))
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

tests/test_state: tests/test_state.c $(wildcard src/*.h) $(filter-out build/main.o build/assetview.o build/tournament.o build/replay.o,$(OBJ)) src/tournament.c src/replay.c
	$(CC) $(CFLAGS) -o $@ tests/test_state.c $(filter-out build/main.o build/assetview.o build/tournament.o build/replay.o,$(OBJ)) $(LDLIBS)

tests/test_net: tests/test_net.c src/net.c src/net.h
	$(CC) $(CFLAGS) -o $@ tests/test_net.c src/net.c $(LDLIBS)

.PHONY: all clean test
