#include <Arduino.h>
#include "Config.h"

using namespace Config;
#include <ctype.h>
#include "OLED_test.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace {
size_t currentScreen = 0;
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastButtonChangeMs = 0;

OLEDTestState activeState = OLEDTestState::Off;
bool initialized = false;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// The built-in font uses 6 x 8 pixel character cells at size 1.
// Pack whole words into lines, returning false if the text cannot fit.
bool wrapWords(const char* text, uint8_t textSize, String* lines, size_t& lineCount) {
  const size_t columns = SCREEN_WIDTH / (6 * textSize);
  const size_t maxLines = SCREEN_HEIGHT / (8 * textSize);
  lineCount = 1;
  for (size_t i = 0; i < SCREEN_HEIGHT / 8; ++i) {
    lines[i] = "";
  }

  while (*text) {
    if (*text == '\n') {
      if (lineCount == maxLines) return false;
      ++lineCount;
      ++text;
      continue;
    }
    if (isspace(static_cast<unsigned char>(*text))) {
      ++text;
      continue;
    }

    String word;
    while (*text && !isspace(static_cast<unsigned char>(*text))) {
      word += *text++;
    }
    if (word.length() > columns) return false;

    const size_t used = lines[lineCount - 1].length();
    if (used && used + 1 + word.length() > columns) {
      if (lineCount == maxLines) return false;
      ++lineCount;
    }
    if (lines[lineCount - 1].length()) lines[lineCount - 1] += ' ';
    lines[lineCount - 1] += word;
  }
  return true;
}
} // namespace

void setOLEDTestState(OLEDTestState state) {
  if (!initialized) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Wire.begin(SDA_PIN, SCL_PIN, I2C_CLOCK_HZ);
    Wire.setTimeOut(I2C_TIMEOUT_MS);

    // Preserve the custom I2C pins instead of restarting Wire.
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, true, false)) {
      Serial.println("OLED initialization failed");
      return;
    }
    initialized = true;
  }

  if (state != activeState) {
    lastButtonReading = digitalRead(BUTTON_PIN);
    stableButtonState = lastButtonReading;
    lastButtonChangeMs = millis();
  }
  activeState = state;

  display.clearDisplay();
  if (state == OLEDTestState::Off) {
    // Clear any text retained from a previous run.
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    return;
  }

  display.ssd1306_command(SSD1306_DISPLAYON);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  String lines[SCREEN_HEIGHT / 8];
  size_t lineCount = 0;
  uint8_t textSize = MAX_TEXT_SIZE;
  bool fits = false;
  for (; textSize > 0; --textSize) {
    if (wrapWords(SCREEN_TEXT[currentScreen], textSize, lines, lineCount)) {
      fits = true;
      break;
    }
  }
  // Only shrink if the whole message (or a single word) cannot fit at MAX_TEXT_SIZE.
  if (!fits) {
    textSize = 1;
    wrapWords("Text too long", textSize, lines, lineCount);
    Serial.println("OLED text cannot fit on this screen without splitting words.");
  }

  display.setTextSize(textSize);
  const int lineHeight = 8 * textSize;
  const int top = (SCREEN_HEIGHT - lineCount * lineHeight) / 2;
  for (size_t i = 0; i < lineCount; ++i) {
    const int left = (SCREEN_WIDTH - lines[i].length() * 6 * textSize) / 2;
    display.setCursor(left, top + i * lineHeight);
    display.print(lines[i]);
  }
  display.display();
}

void updateOLEDTest() {
  if (!initialized || activeState == OLEDTestState::Off) {
    return;
  }

  const unsigned long now = millis();
  const int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonChangeMs = now;
    lastButtonReading = reading;
  }

  // Accept a change only after the input has stayed steady for DEBOUNCE_MS.
  if (reading != stableButtonState && now - lastButtonChangeMs >= DEBOUNCE_MS) {
    stableButtonState = reading;

    // Advance on a press only, not on release or while held down.
    if (stableButtonState == LOW) {
      currentScreen = (currentScreen + 1) % SCREEN_COUNT;
      setOLEDTestState(activeState);
    }
  }
}
