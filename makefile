CPPC = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-c++
CC = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-cc

FLAGS_COMMON_C = -static -static-libgcc 
FLAGS_COMMON_CPP = $(FLAGS_COMMON_C) -std=c++23

SRC_RADIOLIB = \
	$(wildcard ext/RadioLib/src/*.cpp) \
  $(wildcard ext/RadioLib/src/*/*.cpp) \
  $(wildcard ext/RadioLib/src/*/*/*.cpp) 

SRC_LG = \
  ext/lg/lgCtx.c \
  ext/lg/lgDbg.c \
  ext/lg/lgErr.c \
  ext/lg/lgGpio.c \
  ext/lg/lgHdl.c \
  ext/lg/lgI2C.c \
  ext/lg/lgNotify.c \
  ext/lg/lgPthAlerts.c \
  ext/lg/lgPthTx.c \
  ext/lg/lgSerial.c \
  ext/lg/lgSPI.c \
  ext/lg/lgThread.c \
  ext/lg/lgUtil.c 

FLAGS_LG = -I./ext/lg

SRC_MAIN = src/main.cc bin/radiolib.o bin/lg.o
FLAGS_MAIN = -I./ext/RadioLib/src -I./ext/lg -Wall -Wextra -Wpedantic

BIN = bin

build:
	$(CPPC) $(FLAGS_COMMON_CPP) $(FLAGS_MAIN) $(SRC_MAIN) -o bin/bin 

radiolib:
	$(CPPC) $(FLAGS_COMMON_CPP) $(SRC_RADIOLIB) -DRADIOLIB_DEBUG_BASIC=1 -DSerial=stdout  -r -o bin/radiolib.o 

lg:
	$(CC) $(FLAGS_COMMON_C) $(FLAGS_LG) $(SRC_LG) -r -o bin/lg.o

