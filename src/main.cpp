#include <Arduino.h>
#include "Config.h"
#include "OLED_test.h"
#include "I2C_scan.h"
#include "GY30_test.h"

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  switch (APP_STATE) {
    case AppState::Off:
      setOLEDTestState(OLEDTestState::Off);
      break;
    case AppState::OLEDTest:
      setOLEDTestState(OLEDTestState::On);
      break;
    case AppState::I2CScan:
      beginI2CScan();
      break;
    case AppState::GY30Test:
      beginGY30Test();
      break;
  }
}

void loop() {
  switch (APP_STATE) {
    case AppState::Off:
      delay(Config::IDLE_DELAY_MS);
      break;
    case AppState::OLEDTest:
      updateOLEDTest();
      break;
    case AppState::I2CScan:
      updateI2CScan();
      break;
    case AppState::GY30Test:
      updateGY30Test();
      break;
  }
}
