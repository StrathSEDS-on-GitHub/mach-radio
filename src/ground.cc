#include "hal.cc"
#include <array>
#include <asio.hpp>

using asio::ip::udp;

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

int main(int argc, const char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "need an ip address\n");
    return 1;
  }
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

  asio::io_service io;
  auto addr = asio::ip::address::from_string(argv[1]);
  constexpr u16 port = 1234;
  udp::endpoint endpoint(addr, port);
  udp::socket socket(io, endpoint.protocol());

  std::array<u8, 255> rx_buf{0};
  while (true) {
    res = radio.receive(rx_buf.data(), rx_buf.size());
    if (res != RADIOLIB_ERR_NONE) {
      fprintf(stderr, "packet get error: %d\n", res);
      continue;
    }   
    auto buf = asio::buffer(rx_buf.data(), rx_buf.size());
    socket.send(buf); 
  }
}
