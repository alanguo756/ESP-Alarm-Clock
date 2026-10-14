#include <Arduino.h>
#include "Config.h"

using namespace Config;
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "GY30_test.h"

namespace {
uint8_t brightnessForLux(float lux) {
  if (lux <= 0.0f) return 0;
  if (lux >= FULL_BRIGHTNESS_LUX) return 255;
  return static_cast<uint8_t>(lux * 255.0f / FULL_BRIGHTNESS_LUX + 0.5f);
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool busReady = false;
bool displayReady = false;
bool sensorReady = false;
unsigned long lastUpdateMs = 0;
int appliedBrightness = -1;

void applyBrightness(uint8_t brightness) {
  if (!displayReady || appliedBrightness == brightness) return;
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(brightness);
  appliedBrightness = brightness;
}

bool sendCommand(uint8_t command) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

bool startSensor() {
  // BH1750: power on, reset data, set default measurement time (69),
  // then start continuous high-resolution measurements.
  return sendCommand(0x01) && sendCommand(0x07) &&
         sendCommand(0x42) && sendCommand(0x65) && sendCommand(0x10);
}

void showStatus(const char* message) {
  Serial.println(message);
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("GY-30 / BH1750  0x%02X", SENSOR_ADDRESS);
  display.setCursor(0, 24);
  display.println(message);
  display.display();
}
}

void beginGY30Test() {
  appliedBrightness = -1;
  busReady = Wire.begin(SDA_PIN, SCL_PIN, I2C_CLOCK_HZ);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  if (!busReady) {
    Serial.println("GY-30: I2C initialization failed");
    return;
  }

  Wire.beginTransmission(OLED_ADDRESS);
  const bool oledPresent = Wire.endTransmission() == 0;
  displayReady = oledPresent &&
      display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, true, false);
  if (displayReady) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.ssd1306_command(SSD1306_DISPLAYON);
    // Stay dim until the first valid reading.
    applyBrightness(0);
  } else {
    Serial.printf("OLED unavailable at 0x%02X; readings will use Serial only.\n", OLED_ADDRESS);
  }

  sensorReady = startSensor();
  showStatus(sensorReady ? "Waiting for reading" : "Sensor not responding");
  lastUpdateMs = millis();
}

void updateGY30Test() {
  if (!busReady || millis() - lastUpdateMs < UPDATE_MS) return;
  lastUpdateMs = millis();

  if (!sensorReady) {
    sensorReady = startSensor();
    showStatus(sensorReady ? "Waiting for reading" : "Check sensor wiring");
    // Allow a full conversion after starting/reconnecting the sensor.
    lastUpdateMs = millis();
    return;
  }

  if (Wire.requestFrom(SENSOR_ADDRESS, static_cast<uint8_t>(2)) != 2) {
    while (Wire.available()) Wire.read();
    sensorReady = false;
    showStatus("Sensor read failed");
    return;
  }
  const uint16_t highByte = Wire.read();
  const uint16_t raw = (highByte << 8) | Wire.read();
  const float lux = raw / 1.2f;
  const uint8_t brightness = brightnessForLux(lux);
  applyBrightness(brightness);
  Serial.printf("GY-30: raw=%u, %.1f lux, brightness=%u/255%s\n",
                raw, lux, brightness, raw == 0xFFFF ? " (saturated)" : "");

  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("GY-30 / BH1750  0x%02X", SENSOR_ADDRESS);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print(lux, 1);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("lux | OLED: ");
  display.print(brightness);
  display.setCursor(0, 54);
  display.print("Raw: ");
  display.print(raw);
  if (raw == 0xFFFF) display.print(" MAX");
  display.display();
}

void refreshGY30Display() {
  // The clock changes contrast, so force the next sensor update to reapply it.
  appliedBrightness = -1;
  lastUpdateMs = millis() - UPDATE_MS;
}
