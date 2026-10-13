#include <Arduino.h>
#include "Config.h"

using namespace Config;
#include <Wire.h>
#include "I2C_scan.h"

namespace {
unsigned long lastScanMs = 0;
bool ready = false;
}

void beginI2CScan() {
  ready = Wire.begin(SDA_PIN, SCL_PIN, I2C_CLOCK_HZ);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  // Wait briefly for Serial Monitor to connect before the first scan.
  lastScanMs = millis() - SCAN_INTERVAL_MS + SCAN_START_DELAY_MS;
}

void updateI2CScan() {
  const unsigned long now = millis();
  if (now - lastScanMs < SCAN_INTERVAL_MS) return;
  lastScanMs = now;

  if (!ready) {
    Serial.printf("I2C initialization failed on SDA %d / SCL %d.\n", SDA_PIN, SCL_PIN);
    return;
  }

  Serial.printf("\nScanning I2C: SDA GPIO %d, SCL GPIO %d...\n", SDA_PIN, SCL_PIN);
  unsigned int found = 0;
  unsigned int errors = 0;
  // Probe usable 7-bit addresses; skip reserved ranges.
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    const uint8_t result = Wire.endTransmission();
    if (result == 0) {
      Serial.printf("Device found at 0x%02X\n", address);
      ++found;
    } else if (result != 2) {
      ++errors; // An address NACK (2) simply means no device responded.
    }
  }
  Serial.printf("Scan complete: %u device(s) found.\n", found);
  if (errors) {
    Serial.printf("Bus errors/timeouts: %u. Check wiring, power, and pull-ups.\n", errors);
  }
  Serial.printf("Scanning again in %lu ms.\n", SCAN_INTERVAL_MS);
}
