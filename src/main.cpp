#include <Arduino.h>
#include "Config.h"
#include "OLED_test.h"
#include "I2C_scan.h"
#include "GY30_test.h"
#include "WiFiMAC_test.h"
#include "WiFi_test.h"
#include "RTC_test.h"
#include "ClockScreens.h"
#include "Buzzer_test.h"
#include "AlarmSet_test.h"
#include "MainState.h"

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  switch (APP_STATE) {
    case AppState::Main:
      beginMainState();
      break;
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
    case AppState::WiFiMACTest:
      beginWiFiMACTest();
      break;
    case AppState::WiFiTest:
      beginWiFiTest();
      break;
    case AppState::RTCTest:
      beginClockScreens();
      break;
    case AppState::BuzzerTest:
      beginBuzzerTest();
      break;
    case AppState::AlarmSetTest:
      beginAlarmSetTest();
      break;
  }
}

void loop() {
  switch (APP_STATE) {
    case AppState::Main:
      updateMainState();
      break;
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
    case AppState::WiFiMACTest:
      updateWiFiMACTest();
      break;
    case AppState::WiFiTest:
      updateWiFiTest();
      break;
    case AppState::RTCTest:
      updateClockScreens();
      break;
    case AppState::BuzzerTest:
      updateBuzzerTest();
      break;
    case AppState::AlarmSetTest:
      updateAlarmSetTest();
      break;
  }
}
