#include <cassert>
#include <iostream>
#include "AlarmLogic.h"

int main() {
  // Every increment from every valid time must produce another valid time.
  for (unsigned h = 0; h < 24; ++h) {
    for (unsigned m = 0; m < 60; ++m) {
      for (unsigned digit = 0; digit < 4; ++digit) {
        AlarmDigits e;
        e.digits[0] = h / 10; e.digits[1] = h % 10;
        e.digits[2] = m / 10; e.digits[3] = m % 10;
        e.selected = digit;
        e.increment();
        assert(e.hour() < 24 && e.minute() < 60);
      }
    }
  }
  AlarmDigits e;
  e.digits[0] = 2; e.digits[1] = 3; e.selected = 1;
  e.increment(); assert(e.hour() == 20);
  e.digits[2] = 5; e.selected = 2;
  e.increment(); assert(e.digits[2] == 0);
  e.selected = 3; e.next(); assert(e.selected == 0);

  const int32_t offset = -7 * 3600;
  const uint32_t midnight = 20000UL * 86400 + 7 * 3600;
  assert(nextAlarmUtc(midnight, offset, 0, 1) == midnight + 60);
  assert(nextAlarmUtc(midnight, offset, 0, 0) == midnight + 86400);
  assert(nextAlarmUtc(midnight + 12 * 3600 + 30, offset, 12, 0) == midnight + 86400 + 12 * 3600);
  assert(nextAlarmUtc(midnight + 86399, offset, 0, 0) == midnight + 86400);
  assert(nextAlarmUtc(midnight + 86339, offset, 23, 59) == midnight + 86340);
  std::cout << "Alarm digit bounds, wrapping, and next-occurrence scheduling passed.\n";
}
