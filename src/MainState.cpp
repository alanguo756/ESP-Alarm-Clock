#include "MainState.h"
#include "ClockScreens.h"
#include "AutoBrightness.h"

void beginMainState() {
  beginClockScreens(); // Always start on the clock, then cycle weather pages.
  beginAutoBrightness();
}

void updateMainState() {
  updateClockScreens();
  updateAutoBrightness();
}
