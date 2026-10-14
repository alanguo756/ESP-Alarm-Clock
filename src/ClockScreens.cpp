#include <Arduino.h>
#include "Config.h"
#include "ClockScreens.h"
#include "RTC_test.h"
#include "WeatherScreens.h"
#include "AlarmSet_test.h"

namespace {
enum class Screen { Clock, Weather, Today, Tomorrow, Count };
Screen screen = Screen::Clock;
uint32_t rtcTime = 0;
unsigned long lastClockRead = 0;
constexpr bool alarmsEnabled = APP_STATE == AppState::Main;
struct Button {
  int pin;
  int last = HIGH, stable = HIGH;
  unsigned long changed = 0;
  explicit Button(int gpio) : pin(gpio) {}
  void begin() {
    pinMode(pin, INPUT_PULLUP);
    last = stable = digitalRead(pin);
    changed = millis();
  }
  bool pressed(unsigned long now) {
    const int reading = digitalRead(pin);
    if (reading != last) { last = reading; changed = now; }
    if (reading != stable && now - changed >= Config::DEBOUNCE_MS) {
      stable = reading;
      return stable == LOW;
    }
    return false;
  }
};
Button screenButton(Config::BUTTON_PIN), pageButton(Config::PAGE_BUTTON_PIN),
       incrementButton(Config::ALARM_INCREMENT_PIN);

void selectScreen(Screen next) {
  screen = next;
  setRTCDisplayVisible(screen == Screen::Clock && !(alarmsEnabled && rtcStartupComplete()));
  selectWeatherScreen(screen >= Screen::Weather ? static_cast<int>(screen) - static_cast<int>(Screen::Weather) : -1);
  Serial.printf("Screen: %d (0 clock, 1 overview, 2 today, 3 tomorrow)\n", static_cast<int>(screen));
}
}

void beginClockScreens() {
  if (alarmsEnabled) beginAlarmController();
  screenButton.begin(); pageButton.begin();
  if (alarmsEnabled) incrementButton.begin();
  screen = Screen::Clock;
  rtcTime = 0;
  lastClockRead = millis() - 1000;
  beginRTCTest();
}

void updateClockScreens() {
  const unsigned long now = millis();
  const bool nextPressed = screenButton.pressed(now);
  const bool pagePressed = pageButton.pressed(now);
  const bool incrementPressed = alarmsEnabled && incrementButton.pressed(now);
  if (alarmsEnabled && rtcStartupComplete()) setRTCDisplayVisible(false);
  updateRTCTest();

  bool consumed = false;
  if (alarmsEnabled) {
    const bool wasOverlay = alarmOverlayActive();
    const bool wasRinging = alarmIsRinging();
    // Page button edits the clock alarm, but still pages forecasts elsewhere.
    consumed = wasOverlay || (pagePressed && screen == Screen::Clock);
    updateAlarmController(screen == Screen::Clock, pagePressed, nextPressed, incrementPressed);
    consumed = consumed || alarmOverlayActive();
    if ((alarmIsRinging() || wasRinging) && screen != Screen::Clock) selectScreen(Screen::Clock);
  }
  if (!consumed) {
    if (nextPressed) {
      selectScreen(static_cast<Screen>((static_cast<int>(screen) + 1) % static_cast<int>(Screen::Count)));
    } else if (pagePressed) {
      nextWeatherPage();
    }
  }

  if (now - lastClockRead >= 1000) {
    lastClockRead = now;
    rtcTime = rtcUtcNow();
  }
  updateWeatherScreens(rtcStartupComplete(), rtcTime);
}
