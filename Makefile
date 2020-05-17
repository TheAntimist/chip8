CC = g++
CCFLAGS = -std=c++17 -Wall
SDL2FLAGS = -lSDL2 -I/usr/include/SDL2 -D_REENTRANT
BOOSTFLAGS = -lboost_program_options
OUTPUT = -o chip
INPUT = chip.cc chip.hpp

all: build

build:
	@echo "Building chip8..."
	${CC} ${CCFLAGS} ${INPUT} ${OUTPUT}  ${SDL2FLAGS} ${BOOSTFLAGS}
