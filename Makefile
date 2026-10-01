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

.PHONY: all clean
