#include "hal.cc"
#include "asio.hpp"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <array>
#include <thread>

using asio::ip::tcp;

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

int start_stream(char *const *argv)
{
  int fds[2];
  if (pipe(fds) < 0) {
    perror("could not allocate pipe for child output");
    return -1;
  }
  auto [read_fd, write_fd] = fds;
  auto pid = fork();
  if (pid == 0) {
    close(read_fd);
    if (dup2(write_fd, STDOUT_FILENO) == -1) {
      exit(errno);
    }
    int res = execvp(argv[8], argv + 8);
    exit(res);
  } else if (pid > 0) {
    close(write_fd);
    return read_fd;
  } else {
    close(write_fd);
    close(read_fd);
    return -1;
  }
}

int main(int argc, char *const *argv)
{
  if (argc < 9) {
    fprintf(stderr, "usage: rocket <carrier_freq> <bandwidth> <sf> <cr> <sync word> <power> <preamble> <command> [args...] fucking idiot.");
    return 1;
  }
  int stream_fd = start_stream(argv);

  double carrier_freq = std::stod(argv[1]);
  double bandwidth = std::stod(argv[2]);
  int spreading_factor = std::stoi(argv[3]);
  int coding_rate = std::stoi(argv[4]);
  int sync_word = std::stoi(argv[5], nullptr, 16); // Parse as hex
  int output_power = std::stoi(argv[6]);
  int preamble_length = std::stoi(argv[7]);

  auto res = radio.begin(carrier_freq, bandwidth, spreading_factor, coding_rate, sync_word, output_power, preamble_length);

  if (res == RADIOLIB_ERR_NONE) {
    printf("initialisation successful\n");
  } else {
    fprintf(stderr, "initialisation error %d\n", res);
    return 1;
  }
  
  std::array<u8, 255> tx_buf{0};
  while (true) {
    res = read(stream_fd, tx_buf.data(), tx_buf.size());
    if (res == 0) {
      continue;
    } else if (res == -1) {
      perror("Error reading from stream");
      continue;
    }
    res = radio.transmit(tx_buf.data(), tx_buf.size());
    if (res != RADIOLIB_ERR_NONE) {
      fprintf(stderr, "packet send error: %d\n", res);
    }
  }
}

