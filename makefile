CC = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-c++
FLAGS = -std=c++20 -static -static-libgcc -I./ext/RadioLib/src
SRC = $(wildcard src/*.cc)
BIN = bin
RADIOLIB =  $(wildcard ext/RadioLib/src/*.cpp) 
RADIOLIB += $(wildcard ext/RadioLib/src/*/*.cpp) 
RADIOLIB += $(wildcard ext/RadioLib/src/*/*/*.cpp)

build:
	$(CC) $(FLAGS) $(SRC) bin/radiolib.o -o bin/bin 

radiolib:
	$(CC) $(FLAGS) $(RADIOLIB) -r -o bin/radiolib.o 

