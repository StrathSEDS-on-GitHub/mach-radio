#include "base.hh"
#include <cstdio>
#include <RadioLib.h>

struct RockHal : public RadioLibHal
{
  void init() override {}
  void term() override {}
  void pinMode(u32 pin, u32 mode) override {}
  void digitalWrite(u32 pin, u32 value) override {}
  uint32_t digitalRead(uint32_t pin) override {}
  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {}
  void detachInterrupt(uint32_t interruptNum) override {}
  void delay(unsigned long ms) override {}
  void delayMicroseconds(unsigned long us) override {}
  void yield() override {}
  unsigned long millis() override {}
  unsigned long micros() override {}
  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {}
};

int main()
{
}
