namespace rock
{

constexpr int 
  RISING   = LG_RISING_EDGE,
  FALLING  = LG_FALLING_EDGE,
  INPUT    = 0,
  OUTPUT   = 1,
  MAX_GPIO = 31;

enum 
{
  RXEN = 5,
  TXEN = 7,
  NRESET = 11,
  BUSY = 13,
  DIO1 = 15,
  NSS = 24
};

// forward declaration of alert handler that will be used to emulate interrupts
static void lgpioAlertHandler(int num_alerts, lgGpioAlert_p alerts, void *userdata);

// create a new Raspberry Pi hardware abstraction layer
// using the lgpio library
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class Hal : public RadioLibHal {
  int gpio_1_handle;
  int gpio_2_handle;
  int gpio_4_handle;
  int spi_handle;
  std::map<u8, std::tuple<int, u8>> pins;

public:
  // interrupt emulation
  bool interruptEnabled[MAX_GPIO + 1];
  u32 interruptModes[MAX_GPIO + 1];
  typedef void (*RadioLibISR)(void);
  RadioLibISR interruptCallbacks[MAX_GPIO + 1];

  // default constructor - initializes the base HAL and any needed private members
  Hal()
  : RadioLibHal(INPUT, OUTPUT, LG_LOW, LG_HIGH, RISING, FALLING)
  {}

  void init() override 
  {
    gpio_1_handle = lgGpiochipOpen(1);
    if (gpio_1_handle < 0) {
      fprintf(stderr, "Could not open GPIO chip 1: %s\n", lguErrorText(gpio_1_handle));
      return;
    }

    gpio_2_handle = lgGpiochipOpen(2);
    if (gpio_2_handle < 0) {
      fprintf(stderr, "Could not open GPIO chip 2: %s\n", lguErrorText(gpio_2_handle));
      return;
    }

    gpio_4_handle = lgGpiochipOpen(4);
    if (gpio_4_handle < 0) {
      fprintf(stderr, "Could not open GPIO chip 4: %s\n", lguErrorText(gpio_4_handle));
      return;
    }

    spiBegin();

    pins[NSS]    = std::tuple(gpio_1_handle, 10);
    pins[RXEN]   = std::tuple(gpio_2_handle, 8);
    pins[TXEN]   = std::tuple(gpio_2_handle, 11);
    pins[NRESET] = std::tuple(gpio_4_handle, 18);
    pins[BUSY]   = std::tuple(gpio_4_handle, 22);
    pins[DIO1]   = std::tuple(gpio_4_handle, 21);
    
  }

  void term() override 
  {
    lgGpiochipClose(gpio_2_handle);
    lgGpiochipClose(gpio_4_handle);
  }

  void spiBegin() override
  {
    constexpr u32 spi_clock_speed = 1800000;
    constexpr u8 spi_device = 1;
    constexpr u8 spi_channel = 0;

    spi_handle = lgSpiOpen(spi_device, spi_channel, spi_clock_speed, 0);
    if (spi_handle < 0) {
      fprintf(stderr, "Could not open SPI handle on 0: %s\n", lguErrorText(spi_handle));
      return;
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
      result = lgGpioClaimOutput(handle, 0, line, LG_LOW);
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
      fprintf(stderr, "Error writing value to pin %" PRIu32 ": %s\n", pin, lguErrorText(result));
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

  void attachInterrupt(u32 interruptNum, void (*interruptCb)(void), u32 mode) override 
  {
    if (interruptNum == RADIOLIB_NC || interruptNum > MAX_GPIO) {
      return;
    }
    if (!pins.contains(interruptNum)) {
      fprintf(stderr, "Invalid pin requested: %d\n", interruptNum);
      return;
    }
    auto [handle, line] = pins[interruptNum];

    // set lgpio alert callback
    int result = lgGpioClaimAlert(handle, 0, mode, line, -1);
    if (result < 0) {
      fprintf(stderr, "Could not claim pin %" PRIu32 " for alert: %s\n", interruptNum, lguErrorText(result));
      return;
    }

    // enable emulated interrupt
    interruptEnabled[interruptNum] = true;
    interruptModes[interruptNum] = mode;
    interruptCallbacks[interruptNum] = interruptCb;

    lgGpioSetAlertsFunc(handle, line, lgpioAlertHandler, (void *)this);
  }

  void detachInterrupt(u32 interruptNum) override 
  {
    if ((interruptNum == RADIOLIB_NC) || (interruptNum > MAX_GPIO)) {
      return;
    }
    if (!pins.contains(interruptNum)) {
      fprintf(stderr, "Invalid pin requested: %d\n", interruptNum);
      return;
    }
    auto [handle, line] = pins[interruptNum];

    // clear emulated interrupt
    interruptEnabled[interruptNum] = false;
    interruptModes[interruptNum] = 0;
    interruptCallbacks[interruptNum] = NULL;

    // disable lgpio alert callback
    lgGpioFree(handle, line);
    lgGpioSetAlertsFunc(handle, line, NULL, NULL);
  }

  void delay(u64 ms) override 
  {
    if (ms == 0) {
      sched_yield();
      return;
    }

    lguSleep(ms / 1000.0);
  }

  void delayMicroseconds(u64 us) override 
  {
    if (us == 0) {
      sched_yield();
      return;
    }

    lguSleep(us / 1000000.0);
  }

  void yield() override 
  {
    sched_yield();
  }

  u64 millis() override 
  {
    u32 time = lguTimestamp() / 1000000UL;
    return time;
  }

  u64 micros() override 
  {
    u32 time = lguTimestamp() / 1000UL;
    return time;
  }

  i64 pulseIn(u32 pin, u32 state, u64 timeout) override 
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

  void tone(u32 pin, u32 frequency, u64 duration = 0) override
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
};

// this handler emulates interrupts
static void lgpioAlertHandler(int num_alerts, lgGpioAlert_p alerts, void *userdata) 
{
  if (!userdata)
    return;

  // RockHal instance is passed via the user data
  Hal *hal = (Hal*)userdata;

  // check the interrupt is enabled, the level matches and a callback exists
  for (lgGpioAlert_t *alert = alerts; alert < (alerts + num_alerts); alert++) {
    if ((hal->interruptEnabled[alert->report.gpio]) 
    &&  (hal->interruptModes[alert->report.gpio] == alert->report.level) 
    &&  (hal->interruptCallbacks[alert->report.gpio])) {
      hal->interruptCallbacks[alert->report.gpio]();
    }
  }
}

} // namespace rock

