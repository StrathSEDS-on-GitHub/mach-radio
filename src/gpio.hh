#ifndef GPIO_H
#define GPIO_H

#include <type_traits>
#include <gpiod.hpp>

class RockGpio
{
  gpiod::chip gpio1;
  gpiod::chip gpio2;
  gpiod::chip gpio3;
  gpiod::chip gpio4;

  template<u8> struct DependsFalse : std::false_type {};

public:
  RockGpio()
  : gpio1("gpiochip1")
  , gpio2("gpiochip2")
  , gpio3("gpiochip3")
  , gpio4("gpiochip4")
  {}

  template<u8 Pin>
  gpiod::line get_pin()
  {
    gpiod::chip &chip;
    u8 line;

    // Datasheet shorthand for line offsets
    constexpr auto A = [](u8 n) { return n;      };
    constexpr auto B = [](u8 n) { return 8 + n;  };
    constexpr auto C = [](u8 n) { return 16 + n; };
    constexpr auto D = [](u8 n) { return 24 + n; };

    if      constexpr (Pin == 4)  { chip = gpio2; line = A(7); }
    else if constexpr (Pin == 5)  { chip = gpio2; line = B(0); }
    else if constexpr (Pin == 7)  { chip = gpio2; line = B(3); }
    else if constexpr (Pin == 8)  { chip = gpio4; line = C(4); }
    else if constexpr (Pin == 10) { chip = gpio4; line = C(3); }
    else if constexpr (Pin == 11) { chip = gpio4; line = C(2); }
    else if constexpr (Pin == 12) { chip = gpio4; line = A(3); }
    else if constexpr (Pin == 13) { chip = gpio4; line = C(6); }
    else if constexpr (Pin == 15) { chip = gpio4; line = C(5); }
    else if constexpr (Pin == 16) { chip = gpio4; line = D(2); }
    else if constexpr (Pin == 18) { chip = gpio4; line = D(4); }
    else if constexpr (Pin == 19) { chip = gpio1; line = B(0); }
    else if constexpr (Pin == 21) { chip = gpio1; line = A(7); }
    else if constexpr (Pin == 22) { chip = gpio4; line = D(5); }
    else if constexpr (Pin == 23) { chip = gpio1; line = B(1); }
    else if constexpr (Pin == 24) { chip = gpio1; line = B(2); }
    else if constexpr (Pin == 27) { chip = gpio2; line = A(0); }
    else if constexpr (Pin == 28) { chip = gpio2; line = A(1); }
    else if constexpr (Pin == 29) { chip = gpio2; line = B(2); }
    else if constexpr (Pin == 31) { chip = gpio2; line = B(1); }
    else if constexpr (Pin == 32) { chip = gpio3; line = C(0); }
    else if constexpr (Pin == 33) { chip = gpio2; line = B(4); }
    else if constexpr (Pin == 35) { chip = gpio4; line = A(5); }
    else if constexpr (Pin == 36) { chip = gpio4; line = A(4); }
    else if constexpr (Pin == 37) { chip = gpio4; line = D(6); }
    else if constexpr (Pin == 38) { chip = gpio4; line = A(6); }
    else if constexpr (Pin == 40) { chip = gpio4; line = A(7); }
    else {
      static_assert(DependsFalse<Pin>::value, "Invalid pin number");
    }

    return chip.get_line(line);
  }
};

#endif
