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
    NSS = 26
  };
}

auto hal = new LinuxHal<
  Spi{ 0, 0 },
  GpioPin{ pin::RXEN,  0, 3  },
  GpioPin{ pin::TXEN,  0, 4  },
  GpioPin{ pin::NRESET,0, 17 },
  GpioPin{ pin::BUSY,  0, 27 },
  GpioPin{ pin::DIO1,  0, 22 },
  GpioPin{ pin::NSS,   0, 7  }
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

  u8 recv[255] = {0};
  while (true) {
    res = radio.receive(recv, 255);
    if (res == RADIOLIB_ERR_NONE) {
      printf("got packet: %s\n", (char*)recv);
    } else {
      fprintf(stderr, "packet get error: %d\n", res);
    }
    
    hal->delay(100);
  }
}

