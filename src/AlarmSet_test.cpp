#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "RTC_test.h"
#include "AlarmLogic.h"
#include "AlarmSet_test.h"
#include "AutoBrightness.h"

namespace {
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
Button confirm(Config::PAGE_BUTTON_PIN), next(Config::BUTTON_PIN), increment(Config::ALARM_INCREMENT_PIN);
Adafruit_SSD1306 display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, -1);
AlarmDigits editor;
bool initialized = false, displayReady = false, editing = false, armed = false, ringing = false;
uint32_t alarmUtc = 0, currentUtc = 0;
unsigned long lastReadMs = 0;

void render() {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (!currentUtc && !ringing) {
    display.println("RTC time unavailable");
    display.println("Restart with Wi-Fi");
    display.println("to set the clock.");
  } else if (ringing) {
    display.print("ALARM!");
    const DateTime local((currentUtc ? currentUtc : alarmUtc) + Config::RTC_UTC_OFFSET_SECONDS);
    display.setTextSize(2); display.setCursor(16, 20);
    display.printf("%02u:%02u:%02u", local.hour(), local.minute(), local.second());
    display.setTextSize(1); display.setCursor(0, 54);
    display.print("16: stop alarm");
  } else if (editing) {
    display.printf("SET ALARM 24H %s", Config::RTC_TIMEZONE_LABEL);
    display.setTextSize(3); display.setCursor(19, 16);
    display.printf("%02u:%02u", editor.hour(), editor.minute());
    const int digitX[] = {19, 37, 73, 91};
    display.drawFastHLine(digitX[editor.selected], 41, 16, SSD1306_WHITE);
    display.setTextSize(1); display.setCursor(0, 46);
    display.print("1:+digit 6:next");
    display.setCursor(0, 56); display.print("16: save alarm");
  } else {
    display.printf("ALARM CLOCK %s", Config::RTC_TIMEZONE_LABEL);
    const DateTime local(currentUtc + Config::RTC_UTC_OFFSET_SECONDS);
    display.setTextSize(2); display.setCursor(16, 16);
    display.printf("%02u:%02u:%02u", local.hour(), local.minute(), local.second());
    display.setTextSize(1); display.setCursor(0, 38);
    if (armed) {
      const DateTime alarm(alarmUtc + Config::RTC_UTC_OFFSET_SECONDS);
      display.printf("Alarm %02u:%02u %02u/%02u", alarm.hour(), alarm.minute(), alarm.month(), alarm.day());
    } else display.print("Alarm: off");
    display.setCursor(0, 54); display.print("16: set alarm");
  }
  display.display();
}
}

void beginAlarmController() {
  pinMode(Config::BUZZER_PIN, OUTPUT);
  digitalWrite(Config::BUZZER_PIN, LOW);
  confirm.begin(); next.begin(); increment.begin();
  initialized = displayReady = editing = armed = ringing = false;
  currentUtc = alarmUtc = 0;
  editor = AlarmDigits{};

}

bool alarmOverlayActive() { return editing || ringing; }
bool alarmIsRinging() { return ringing; }

void updateAlarmController(bool clockVisible, bool savePressed, bool nextPressed, bool incrementPressed) {
  const unsigned long now = millis();
  if (!rtcStartupComplete()) return;
  if (!initialized) {
    Wire.beginTransmission(Config::OLED_ADDRESS);
    displayReady = Wire.endTransmission() == 0 &&
        display.begin(SSD1306_SWITCHCAPVCC, Config::OLED_ADDRESS, true, false);
    if (displayReady) {
      display.setTextColor(SSD1306_WHITE);
      display.setTextWrap(false);
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(currentOLEDBrightness());
    }
    initialized = true;
    lastReadMs = now - Config::ALARM_POLL_MS;
  }
  bool dirty = false;
  if (now - lastReadMs >= Config::ALARM_POLL_MS || savePressed) {
    lastReadMs = now;
    currentUtc = rtcUtcNow();
    dirty = true;
  }
  // A >= comparison also catches a missed polling tick or brief RTC read error.
  if (armed && currentUtc && currentUtc >= alarmUtc) {
    armed = false; editing = false; ringing = true;
    tone(Config::BUZZER_PIN, Config::BUZZER_FREQUENCY_HZ);
    Serial.println("Alarm ringing; press GPIO 16 to stop");
    dirty = true;
  }
  if (savePressed && (clockVisible || ringing || editing)) {
    if (ringing) {
      noTone(Config::BUZZER_PIN);
      digitalWrite(Config::BUZZER_PIN, LOW);
      ringing = false;
      Serial.println("Alarm stopped (one-shot alarm disarmed)");
    } else if (currentUtc) {
      if (editing) {
        alarmUtc = nextAlarmUtc(currentUtc, Config::RTC_UTC_OFFSET_SECONDS, editor.hour(), editor.minute());
        armed = true; editing = false;
        Serial.printf("Alarm set: %02u:%02u %s, next occurrence (UTC epoch %lu)\n",
                      editor.hour(), editor.minute(), Config::RTC_TIMEZONE_LABEL, static_cast<unsigned long>(alarmUtc));
      } else {
        editing = true;
        editor.selected = 0;
        if (!armed) {
          const DateTime local(currentUtc + Config::RTC_UTC_OFFSET_SECONDS);
          editor.digits[0] = local.hour() / 10; editor.digits[1] = local.hour() % 10;
          editor.digits[2] = local.minute() / 10; editor.digits[3] = local.minute() % 10;
        }
      }
    } else Serial.println("Cannot set alarm: RTC time invalid");
    dirty = true;
  } else if (editing && !ringing) {
    if (nextPressed) { editor.next(); dirty = true; }
    if (incrementPressed) { editor.increment(); dirty = true; }
  }
  if (dirty && (clockVisible || editing || ringing)) render();
}

void beginAlarmSetTest() {
  beginAlarmController();
  beginRTCTest();
}

void updateAlarmSetTest() {
  const unsigned long now = millis();
  const bool savePressed = confirm.pressed(now);
  const bool nextPressed = next.pressed(now);
  const bool incrementPressed = increment.pressed(now);
  updateRTCTest();
  if (rtcStartupComplete()) setRTCDisplayVisible(false);
  updateAlarmController(true, savePressed, nextPressed, incrementPressed);
}
