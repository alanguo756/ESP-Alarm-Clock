#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "AutoBrightness.h"

namespace {
bool sensorReady = false;
unsigned long lastReadMs = 0;
uint8_t brightness = Config::CLOCK_BRIGHTNESS;
int appliedBrightness = -1;

bool command(uint8_t value) {
  Wire.beginTransmission(Config::SENSOR_ADDRESS);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool startSensor() {
  return command(0x01) && command(0x07) && command(0x42) &&
         command(0x65) && command(0x10);
}
}

uint8_t currentOLEDBrightness() { return brightness; }

void beginAutoBrightness() {
  // Clock setup has already initialized the shared I2C bus and OLED.
  // Only initialize the sensor; do not clear or reinitialize the screen.
  brightness = Config::CLOCK_BRIGHTNESS;
  appliedBrightness = -1;
  sensorReady = startSensor();
  lastReadMs = millis();
  Serial.println(sensorReady ? "Automatic brightness enabled" :
                 "Light sensor unavailable; keeping default brightness and retrying");
}

void updateAutoBrightness() {
  if (millis() - lastReadMs < Config::UPDATE_MS) return;
  lastReadMs = millis();
  if (!sensorReady) {
    sensorReady = startSensor();
    return; // Allow a complete sensor conversion before reading.
  }
  if (Wire.requestFrom(Config::SENSOR_ADDRESS, static_cast<uint8_t>(2)) != 2) {
    while (Wire.available()) Wire.read();
    sensorReady = false;
    Serial.println("Light sensor read failed; retaining brightness");
    return;
  }
  const uint16_t high = Wire.read();
  const uint16_t raw = (high << 8) | Wire.read();
  const float lux = raw / 1.2f;
  brightness = lux >= Config::FULL_BRIGHTNESS_LUX ? 255 :
      static_cast<uint8_t>(lux * 255.0f / Config::FULL_BRIGHTNESS_LUX + 0.5f);
  if (brightness < Config::MIN_OLED_BRIGHTNESS) brightness = Config::MIN_OLED_BRIGHTNESS;
  if (appliedBrightness == brightness) return;
  Wire.beginTransmission(Config::OLED_ADDRESS);
  Wire.write(0x00); // SSD1306 command stream, not display pixels.
  Wire.write(0x81); // Set contrast.
  Wire.write(brightness);
  if (Wire.endTransmission() == 0) appliedBrightness = brightness;
}
