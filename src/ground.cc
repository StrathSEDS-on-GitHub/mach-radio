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

int main(int argc, const char **argv)
{
  if (argc != 8) {
    fprintf(stderr, "usage: ground <carrier_freq> <bandwidth> <sf> <cr> <sync word> <power> <preamble>");
    return 1;
  }
  double carrier_freq = std::stod(argv[1]);
  double bandwidth = std::stod(argv[2]);
  int spreading_factor = std::stoi(argv[3]);
  int coding_rate = std::stoi(argv[4]);
  int sync_word = std::stoi(argv[5], nullptr, 16); // Parse as hex
  int output_power = std::stoi(argv[6]);
  int preamble_length = std::stoi(argv[7]);

  // Simulating the call to radio.begin
  auto res = radio.begin(carrier_freq, bandwidth, spreading_factor, coding_rate, sync_word, output_power, preamble_length);
  if (res == RADIOLIB_ERR_NONE) {
    fprintf(stderr, "initialisation successful\n");
  } else {
    fprintf(stderr, "initialisation error %d\n", res);
    return 1;
  }

  std::array<u8, 255> rx_buf{0};
  while (true) {
    res = radio.receive(rx_buf.data(), rx_buf.size());
    if (res != RADIOLIB_ERR_NONE && res != RADIOLIB_ERR_RX_TIMEOUT) {
      fprintf(stderr, "packet get error: %d\n", res);
      continue;
    }   
    auto buf = asio::buffer(rx_buf.data(), rx_buf.size());

    printf("%s", std::string(static_cast<char*>(buf.data())).c_str());
  }
}

