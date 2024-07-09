#include "hal.cc"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
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
    int res = execvp(*argv, argv+1);
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
  if (argc < 2) {
    fprintf(stderr, "need a streaming command\n");
    return 1;
  }
  bool staging = true;
  {
    constexpr u16 port = 1111;
    asio::io_service io;
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    tcp::acceptor acceptor(io_context, endpoint);
    auto start_session = [&](tcp::socket sock) {
      // do stuff
    };
    while (staging) {
      std::thread(start_session, acceptor.accept());
    }
  }
  int stream_fd = start_stream(argv);
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

