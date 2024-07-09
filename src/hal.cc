#include <RadioLib.h>
#include <lgpio.h>
#include <map>
#include <tuple>
#include <cstdio>
#include <cstdint>
#include <cstddef>

using i8    = int8_t;
using i16   = int16_t;
using i32   = int32_t;
using i64   = int64_t;
using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using f32   = float;
using f64   = double;
using usize = size_t;

constexpr u8 
  RISING   = LG_RISING_EDGE,
  FALLING  = LG_FALLING_EDGE,
  INPUT    = 0,
  OUTPUT   = 1,
  LOW      = LG_LOW, 
  HIGH     = LG_HIGH;

struct GpioPin
{
  u8 pin, chip, line;
};

struct Spi
{
  u8 device, channel;
};

struct GpioInterrupt
{
  typedef void (*RadioLibISR)(void);
  bool enabled = false;
  u32 mode = 0;
  RadioLibISR callback = nullptr;
};

template<Spi S, GpioPin ...P>
class LinuxHal : public RadioLibHal
{
  std::map<u8, int> gpio_handles;
  std::map<u8, std::tuple<int, u8>> pins;
  int spi_handle;

public:
  std::map<u8, GpioInterrupt> interrupts;
  
  LinuxHal()
  : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING)
  {}

  void init() override 
  {
    ([&](){
      int handle;
      if (!gpio_handles.contains(P.chip)) {
        handle = lgGpiochipOpen(P.chip);
        if (handle < 0) {
          fprintf(stderr, "Could not open GPIO chip 1: %s\n", lguErrorText(handle));
          return;
        }
        gpio_handles[P.chip] = handle;
      } else {
        handle = gpio_handles[P.chip];
      }
      pins[P.pin] = std::tuple(handle, P.line);
      interrupts[P.pin] = GpioInterrupt{};
    }(), ...);

    spiBegin();
  }

  void term() override 
  {
    for (auto const &[_, handle] : gpio_handles) {
      lgGpiochipClose(handle);
    }
  }

  void spiBegin() override
  {
    constexpr u32 spi_clock_speed = 1800000;

    spi_handle = lgSpiOpen(S.device, S.channel, spi_clock_speed, 0);
    if (spi_handle < 0) {
      fprintf(stderr, "Could not open SPI handle on 0: %s\n", lguErrorText(spi_handle));
    }
  }

  void spiEnd() override 
  {
    lgSpiClose(spi_handle);
  }

  // GPIO-related methods (pinMode, digitalWrite etc.) should check
  // RADIOLIB_NC as an alias for non-connected pins
  void pinMode(u32 pin, u32 mode) override 
  {
    if (pin == RADIOLIB_NC) {
      return;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }

    auto [handle, line] = pins[pin];

    int result;
    switch (mode) {
    case INPUT:
      result = lgGpioClaimInput(handle, 0, line);
      break;
    case OUTPUT:
      result = lgGpioClaimOutput(handle, 0, line, LOW);
      break;
    default:
      fprintf(stderr, "Unknown pinMode mode %" PRIu32 "\n", mode);
      return;
    }

    if (result < 0) {
      fprintf(
        stderr, 
        "Could not claim pin %" PRIu32 " for mode %" PRIu32 ": %s\n",
        pin, 
        mode, 
        lguErrorText(result)
      );
    }
  }

  void digitalWrite(u32 pin, u32 value) override 
  {
    if (pin == RADIOLIB_NC) {
      return;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }

    auto [handle, line] = pins[pin];

    int result = lgGpioWrite(handle, line, value);

    if (result < 0) {
      fprintf(
        stderr, 
        "Error writing to pin %" PRIu32 ": %s\n", pin, lguErrorText(result)
      );
    }
  }

  u32 digitalRead(u32 pin) override 
  {
    if (pin == RADIOLIB_NC) {
      return 0;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return 0;
    }

    auto [handle, line] = pins[pin];

    int result = lgGpioRead(handle, line);
    if (result < 0) {
      fprintf(stderr, "Error writing reading from pin %" PRIu32 ": %s\n", pin, lguErrorText(result));
    }
    return result;
  }

  void attachInterrupt(u32 pin, void (*callback)(void), u32 mode) override 
  {
    if (pin == RADIOLIB_NC) {
      return;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }
    auto [handle, line] = pins[pin];

    int result = lgGpioClaimAlert(handle, 0, mode, line, -1);
    if (result < 0) {
      fprintf(
        stderr, 
        "Could not claim pin %" PRIu32 " for alert: %s\n", pin, lguErrorText(result)
      );
      return;
    }

    interrupts[pin] = GpioInterrupt{ true, mode, callback };

    constexpr auto handle_alert = [](int num_alerts, lgGpioAlert_t *alerts, void *data) {
      auto hal = static_cast<LinuxHal<S, P...> *>(data);
      for (int i = 0; i < num_alerts; ++i) {
        auto report = alerts[i].report;
        auto &interrupt = hal->interrupts[report.gpio];
        if (interrupt.enabled && interrupt.mode == report.level) {
          interrupt.callback();
        }
      }
    };

    lgGpioSetAlertsFunc(handle, line, handle_alert, (void*)this);
  }

  void detachInterrupt(u32 pin) override 
  {
    if (pin == RADIOLIB_NC) {
      return;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }

    auto [handle, line] = pins[pin];
    interrupts[pin] = GpioInterrupt{};
    lgGpioFree(handle, line);
    lgGpioSetAlertsFunc(handle, line, nullptr, nullptr);
  }

  void delay(RadioLibTime_t ms) 
  {
    if (ms == 0) {
      sched_yield();
    } else {
      lguSleep(ms / 1000.0);
    }
  }

  void delayMicroseconds(RadioLibTime_t us) 
  {
    if (us == 0) {
      sched_yield();
    } else {
      lguSleep(us / 1000000.0);
    }
  }

  void yield() override 
  {
    sched_yield();
  }

  RadioLibTime_t millis() 
  {
    auto time = lguTimestamp() / 1000000UL;
    return time;
  }

  RadioLibTime_t micros() 
  {
    auto time = lguTimestamp() / 1000UL;
    return time;
  }

  long pulseIn(u32 pin, u32 state, RadioLibTime_t timeout)
  {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    pinMode(pin, INPUT);
    u32 start = micros();
    u32 curtick = micros();

    while (digitalRead(pin) == state) {
      if ((micros() - curtick) > timeout) {
        return 0;
      }
    }

    return micros() - start;
  }

  void spiBeginTransaction() override {}

  void spiTransfer(u8 *out, usize len, u8 *in) override
  {
    int result = lgSpiXfer(spi_handle, (char *)out, (char*)in, len);
    if (result < 0) {
      fprintf(stderr, "Could not perform SPI transfer: %s\n", lguErrorText(result));
    }
  }

  void spiEndTransaction() override {}

  void tone(u32 pin, u32 frequency, u64 duration = 0)
  {
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }
    auto [handle, line] = pins[pin];
    lgTxPwm(handle, line, frequency, 50, 0, duration);
  }

  void noTone(uint32_t pin) override
  {
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return;
    }
    auto [handle, line] = pins[pin];
    lgTxPwm(handle, line, 0, 0, 0, 0);
  }

private:

  bool using_pin(u8 pin, auto &&callback) {
    if (pin == RADIOLIB_NC) {
      return false;
    }
    if (!pins.contains(pin)) {
      fprintf(stderr, "Invalid pin requested: %d\n", pin);
      return false;
    }
    callback(pin);
    return true;
  }
};

