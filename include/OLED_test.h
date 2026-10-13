#pragma once

enum class OLEDTestState {
  Off,
  On
};

// Initialize the test and switch the display on or off.
void setOLEDTestState(OLEDTestState state);

// Call each loop to debounce the button and cycle through the screens.
void updateOLEDTest();
