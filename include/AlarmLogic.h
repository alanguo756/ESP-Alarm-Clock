#pragma once
#include <stdint.h>

struct AlarmDigits {
  uint8_t digits[4] = {0, 7, 0, 0};
  uint8_t selected = 0;
  unsigned hour() const { return digits[0] * 10 + digits[1]; }
  unsigned minute() const { return digits[2] * 10 + digits[3]; }
  void next() { selected = (selected + 1) % 4; }
  void increment() {
    const unsigned limit = selected == 0 ? 2 :
        selected == 1 && digits[0] == 2 ? 3 : selected == 2 ? 5 : 9;
    digits[selected] = (digits[selected] + 1) % (limit + 1);
    if (digits[0] == 2 && digits[1] > 3) digits[1] = 3;
  }
};

// Schedule strictly after now, using the same fixed local offset as the clock.
inline uint32_t nextAlarmUtc(uint32_t now, int32_t offset, unsigned hour, unsigned minute) {
  const int64_t local = static_cast<int64_t>(now) + offset;
  int64_t target = local - local % 86400 + hour * 3600 + minute * 60;
  if (target <= local) target += 86400;
  return static_cast<uint32_t>(target - offset);
}
