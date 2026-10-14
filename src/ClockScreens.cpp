#include <Arduino.h>
#include "Config.h"
#include "ClockScreens.h"
#include "RTC_test.h"
#include "WeatherScreens.h"

namespace {
enum class Screen { Clock, Weather, Today, Tomorrow, Count };
Screen screen = Screen::Clock;
uint32_t rtcTime = 0;
unsigned long lastClockRead = 0;
int lastReading = HIGH;
int stableReading = HIGH;
unsigned long changedMs = 0;
int lastPageReading = HIGH;
int stablePageReading = HIGH;
unsigned long pageChangedMs = 0;
}

void beginClockScreens() {
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  lastReading = stableReading = digitalRead(Config::BUTTON_PIN);
  changedMs = millis();
  pinMode(Config::PAGE_BUTTON_PIN, INPUT_PULLUP);
  lastPageReading = stablePageReading = digitalRead(Config::PAGE_BUTTON_PIN);
  pageChangedMs = millis();
  screen = Screen::Clock;
  rtcTime = 0;
  lastClockRead = millis() - 1000;
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
      Serial.printf("Screen: %d (0 clock, 1 overview, 2 today, 3 tomorrow)\n", static_cast<int>(screen));
    }
  }

  const int pageReading = digitalRead(Config::PAGE_BUTTON_PIN);
  if (pageReading != lastPageReading) {
    lastPageReading = pageReading;
    pageChangedMs = now;
  }
  if (pageReading != stablePageReading && now - pageChangedMs >= Config::DEBOUNCE_MS) {
    stablePageReading = pageReading;
    if (stablePageReading == LOW) nextWeatherPage();
  }

  // Finish NTP setup even when a weather screen is selected.
  // The hidden clock never writes to the OLED.
  updateRTCTest();
  if (now - lastClockRead >= 1000) {
    lastClockRead = now;
    rtcTime = rtcUtcNow();
  }
  updateWeatherScreens(rtcStartupComplete(), rtcTime);
}
