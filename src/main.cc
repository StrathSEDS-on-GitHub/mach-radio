#include <RadioLib.h>
#include <lgpio.h>
#include <map>
#include <tuple>
#include "base.cc"
#include "rock.cc"

auto hal = new rock::Hal;

auto radio = SX1280(new Module(
  hal, 
  rock::NSS,
  rock::DIO1,
  rock::NRESET,
  rock::BUSY
));

int main()
{
  auto res = radio.begin(
    // 2450.0, //  carrier freq (MHz)
    // 1625.0, //  bandwidth (kHz)
    // 7,      //  spreading factor #
    // 5,      //  coding rate 
    // 0x12,   //  sync word
    // 2,      //  output power (dBm)
    // 20      //  preamble length (symbols)
  );
  if (res != RADIOLIB_ERR_NONE) {
    fprintf(stderr, "initialisation error %d\n", res);
    //return 1;
  }
  hal->pinMode(rock::BUSY, rock::OUTPUT);
  hal->pinMode(rock::TXEN, rock::OUTPUT);
  hal->digitalWrite(rock::TXEN, 1);
  auto msg = "test !!!!!";
  res = radio.transmit(msg);
  if (res != RADIOLIB_ERR_NONE) {
    fprintf(stderr, "transmission error %d\n", res);
    return 1;
  }
}

