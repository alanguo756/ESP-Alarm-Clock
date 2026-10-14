#include <Arduino.h>
#include "Config.h"
#include "ClockScreens.h"
#include "RTC_test.h"
#include "GY30_test.h"
#include "WeatherScreens.h"

namespace {
enum class Screen { Clock, Light, Weather, Today, Tomorrow, Count };
Screen screen = Screen::Clock;
uint32_t rtcTime = 0;
unsigned long lastClockRead = 0;
bool lightTestStarted = false;
int lastReading = HIGH;
int stableReading = HIGH;
unsigned long changedMs = 0;
}

void beginClockScreens() {
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  lastReading = stableReading = digitalRead(Config::BUTTON_PIN);
  changedMs = millis();
  screen = Screen::Clock;
  rtcTime = 0;
  lastClockRead = millis() - 1000;
  lightTestStarted = false;
  beginRTCTest();
}

void updateClockScreens() {
  const unsigned long now = millis();
  const int reading = digitalRead(Config::BUTTON_PIN);
  if (reading != lastReading) {
    lastReading = reading;
    changedMs = now;
  }
  if (reading != stableReading && now - changedMs >= Config::DEBOUNCE_MS) {
    stableReading = reading;
    if (stableReading == LOW) {
      screen = static_cast<Screen>((static_cast<int>(screen) + 1) % static_cast<int>(Screen::Count));
      setRTCDisplayVisible(screen == Screen::Clock);
      selectWeatherScreen(screen >= Screen::Weather ? static_cast<int>(screen) - static_cast<int>(Screen::Weather) : -1);
      if (screen == Screen::Light) {
        if (!lightTestStarted) {
          beginGY30Test();
          lightTestStarted = true;
        } else {
          refreshGY30Display();
        }
      }
      Serial.printf("Screen: %d (0 clock, 1 GY-30, 2 overview, 3 today, 4 tomorrow)\n", static_cast<int>(screen));
    }
  }

  // Finish NTP setup even when the sensor screen is selected.
  // The hidden clock never writes to the OLED.
  updateRTCTest();
  if (screen == Screen::Light) updateGY30Test();
  if (now - lastClockRead >= 1000) {
    lastClockRead = now;
    rtcTime = rtcUtcNow();
  }
  updateWeatherScreens(rtcStartupComplete(), rtcTime);
}
