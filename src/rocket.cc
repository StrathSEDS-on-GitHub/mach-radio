#include "hal.cc"

namespace pin
{
  enum 
  {
    RXEN = 5,
    TXEN = 7,
    NRESET = 11,
    BUSY = 13,
    DIO1 = 15,
    NSS = 24
  };
}

auto hal = new LinuxHal<
  Spi{ 1, 0 },
  GpioPin{ pin::RXEN,  2, 8  },
  GpioPin{ pin::TXEN,  2, 11 },
  GpioPin{ pin::NRESET,4, 18 },
  GpioPin{ pin::BUSY,  4, 22 },
  GpioPin{ pin::DIO1,  4, 21 },
  GpioPin{ pin::NSS,   1, 10 }
>;

auto radio = SX1280(new Module(
  hal, 
  pin::NSS,
  pin::DIO1,
  pin::NRESET,
  pin::BUSY
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
  if (res == RADIOLIB_ERR_NONE) {
    printf("initialisation successful\n");
  } else {
    fprintf(stderr, "initialisation error %d\n", res);
    return 1;
  }

  char send[255] = "PLEASE work";
  while (true) {
    res = radio.transmit(send);
    if (res == RADIOLIB_ERR_NONE) {
      printf("sent packet: %s\n", send);
    } else {
      fprintf(stderr, "packet send error: %d\n", res);
    }
    
    hal->delay(100);
  }
}

