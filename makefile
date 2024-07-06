CPPC = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-c++
CC = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-cc

FLAGS_COMMON_CPP = -std=c++20 -static -static-libgcc 
FLAGS_COMMON_C = -static -static-libgcc 

SRC_RADIOLIB =  $(wildcard ext/RadioLib/src/*.cpp) 
SRC_RADIOLIB += $(wildcard ext/RadioLib/src/*/*.cpp) 
SRC_RADIOLIB += $(wildcard ext/RadioLib/src/*/*/*.cpp)

SRC_GPIOD_CPP = $(wildcard ext/libgpiod/bindings/cxx/*.cpp)
FLAGS_GPIOD_CPP = -I./ext/libgpiod/include -I./ext/libgpiod/bindings/cxx


SRC_GPIOD_C = $(wildcard ext/libgpiod/lib/*.c)
FLAGS_GPIOD_C = -I./ext/libgpiod/include -DGPIOD_VERSION_STR="\"\"" -D_GNU_SOURCE=1

SRC_MAIN =  $(wildcard src/*.cc)
SRC_MAIN += bin/radiolib.o bin/gpiodcpp.o
FLAGS_MAIN = -I./ext/RadioLib/src -I./ext/libgpiod/bindings/cxx -I./ext/libgpiod/include

BIN = bin

build:
	$(CPPC) $(FLAGS_COMMON_CPP) $(FLAGS_MAIN) $(SRC_MAIN) -o bin/bin 

radiolib:
	$(CPPC) $(FLAGS_COMMON_CPP) $(SRC_RADIOLIB) -r -o bin/radiolib.o 

gpiod:
	$(CC) $(FLAGS_COMMON_C) $(FLAGS_GPIOD_C) $(SRC_GPIOD_C) -r -o bin/gpiod.o
	$(CPPC) $(FLAGS_COMMON_CPP) $(FLAGS_GPIOD_CPP) $(SRC_GPIOD_CPP) bin/gpiod.o -r -o bin/gpiodcpp.o
