CPPC_AARCH64 = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-c++
CC_AARCH64 = ~/aarch64-linux-musl-cross/bin/aarch64-linux-musl-cc
CPPC_ARM6 = ~/armv6-linux-musleabihf-cross/bin/armv6-linux-musleabihf-c++
CC_ARM6 = ~/armv6-linux-musleabihf-cross/bin/armv6-linux-musleabihf-cc

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

OBJECTS_ARM6 = bin/radiolib_arm6.o bin/lg_arm6.o
OBJECTS_AARCH64 = bin/radiolib_aarch64.o bin/lg_aarch64.o
FLAGS_MAIN = -I./ext/RadioLib/src -I./ext/lg -Wall -Wextra -Wpedantic

build:
	make rocket
	make groundstation

rocket:
	$(CPPC_AARCH64) $(FLAGS_COMMON_CPP) $(FLAGS_MAIN) $(OBJECTS_AARCH64) src/rocket.cc  -o bin/rocket

groundstation:
	$(CPPC_ARM6) $(FLAGS_COMMON_CPP) $(FLAGS_MAIN) $(OBJECTS_ARM6) src/ground.cc  -o bin/ground

radiolib:
	$(CPPC_AARCH64) $(FLAGS_COMMON_CPP) $(SRC_RADIOLIB) -DRADIOLIB_DEBUG_BASIC=1 -DSerial=stdout  -r -o bin/radiolib_aarch64.o 
	$(CPPC_ARM6) $(FLAGS_COMMON_CPP) $(SRC_RADIOLIB) -DRADIOLIB_DEBUG_BASIC=1 -DSerial=stdout  -r -o bin/radiolib_arm6.o 

lg:
	$(CC_AARCH64) $(FLAGS_COMMON_C) $(FLAGS_LG) $(SRC_LG) -r -o bin/lg_aarch64.o
	$(CC_ARM6) $(FLAGS_COMMON_C) $(FLAGS_LG) $(SRC_LG) -r -o bin/lg_arm6.o

