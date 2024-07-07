#include "base.hh"
#include <cstdio>
#include <RadioLib.h>


struct RockHal : public RadioLibHal
{
  void init() override {}
  void term() override {}
  void pinMode(u32 pin, u32 mode) override {}
  void digitalWrite(u32 pin, u32 value) override {}
  uint32_t digitalRead(u32 pin) override {}
  void attachInterrupt(u32 interruptNum, void (*interruptCb)(void), u32 mode) override {}
  void detachInterrupt(u32 interruptNum) override {}
  void delay(u64 ms) override {}
  void delayMicroseconds(u64 us) override {}
  void yield() override {}
  unsigned long millis() override {}
  unsigned long micros() override {}
  long pulseIn(u32 pin, u32 state, u64 timeout) override {}
};

int main()
{
}
