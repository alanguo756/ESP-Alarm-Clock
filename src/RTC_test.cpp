#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_sntp.h>
#include <time.h>
#include "Config.h"
#include "WiFiSecrets.h"
#include "RTC_test.h"
#include "AutoBrightness.h"

namespace {
RTC_DS3231 rtc;
Adafruit_SSD1306 display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, -1);
bool displayReady = false;
bool displayVisible = true;
bool ntpSynced = false;

void showStatus(const char* message) {
  if (!displayReady || !displayVisible) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setCursor(0, 0);
  display.printf("DS3231 CLOCK (%s)", Config::RTC_TIMEZONE_LABEL);
  display.setCursor(0, 20);
  display.print(message);
  display.display();
}

void showTime(const DateTime& current) {
  if (!displayReady || !displayVisible) return;
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("DS3231 CLOCK (%s)", Config::RTC_TIMEZONE_LABEL);
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.printf("%02u:%02u:%02u", current.hour(), current.minute(), current.second());
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("%04u-%02u-%02u", current.year(), current.month(), current.day());
  display.setCursor(0, 54);
  display.print(ntpSynced ? "NTP synced | RTC" : "RTC | NTP sync failed");
  display.display();
}
enum class Phase { Connecting, Syncing, Running, Missing };
Phase phase = Phase::Missing;
unsigned long phaseStartedMs = 0;
unsigned long lastPrintMs = 0;
unsigned long lastWiFiStatusMs = 0;
int previousWiFiStatus = -1;
int previousWiFiMode = -1;
const char* syncResult = "Startup sync pending";
static_assert(Config::TIME_ADDRESS == 0x68, "DS3231 has fixed I2C address 0x68");

const char* wifiStatusName() {
  if (WiFi.getMode() == WIFI_OFF) return "OFF";
  switch (WiFi.status()) {
    case WL_CONNECTED: return "CONNECTED";
    case WL_NO_SSID_AVAIL: return "NETWORK NOT FOUND";
    case WL_CONNECT_FAILED: return "CONNECTION FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION LOST";
    case WL_DISCONNECTED: return "DISCONNECTED / CONNECTING";
    default: return "CONNECTING";
  }
}

void printWiFiStatus(bool force = false) {
  const int status = WiFi.status();
  const int mode = WiFi.getMode();
  const unsigned long now = millis();
  if (!force && status == previousWiFiStatus && mode == previousWiFiMode &&
      now - lastWiFiStatusMs < Config::WIFI_STATUS_INTERVAL_MS) return;
  previousWiFiStatus = status;
  previousWiFiMode = mode;
  lastWiFiStatusMs = now;
  Serial.printf("[Wi-Fi] %s", wifiStatusName());
  if (mode != WIFI_OFF && status == WL_CONNECTED) {
    Serial.printf(" | IP: %s", WiFi.localIP().toString().c_str());
  }
  Serial.println();
}

bool rtcResponds() {
  Wire.beginTransmission(Config::TIME_ADDRESS);
  return Wire.endTransmission() == 0;
}

void finishSync(const char* result) {
  syncResult = result;
  Serial.println(result);
  showStatus(result);
  sntp_stop();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  printWiFiStatus(true);
  phase = Phase::Running;
  lastPrintMs = millis() - Config::RTC_PRINT_INTERVAL_MS;
}
}

void beginRTCTest() {
  displayVisible = true;
  displayReady = false;
  ntpSynced = false;
  phase = Phase::Missing;
  if (!Wire.begin(Config::SDA_PIN, Config::SCL_PIN, Config::I2C_CLOCK_HZ)) {
    syncResult = "RTC: I2C initialization failed";
    return;
  }
  Wire.setTimeOut(Config::I2C_TIMEOUT_MS);
  Wire.beginTransmission(Config::OLED_ADDRESS);
  const bool oledPresent = Wire.endTransmission() == 0;
  displayReady = oledPresent &&
      display.begin(SSD1306_SWITCHCAPVCC, Config::OLED_ADDRESS, true, false);
  if (displayReady) {
    display.setTextColor(SSD1306_WHITE);
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(currentOLEDBrightness());
  } else {
    Serial.println("Clock OLED unavailable; using Serial Monitor only");
  }
  if (!rtc.begin(&Wire)) {
    syncResult = "DS3231 not found at 0x68; check wiring";
    showStatus(syncResult);
    return;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WiFiSecrets::SSID, WiFiSecrets::PASSWORD);
  printWiFiStatus(true);
  phase = Phase::Connecting;
  phaseStartedMs = millis();
  Serial.println("RTC: connecting to Wi-Fi for startup NTP sync...");
  showStatus("Connecting to Wi-Fi...");
}

void setRTCDisplayVisible(bool visible) {
  displayVisible = visible;
  if (!visible) return;
  if (displayReady) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(currentOLEDBrightness());
  }
  if (phase == Phase::Connecting) showStatus("Connecting to Wi-Fi...");
  else if (phase == Phase::Syncing) showStatus("Wi-Fi connected.\nSyncing NTP...");
  lastPrintMs = millis() - Config::RTC_PRINT_INTERVAL_MS;
}

void updateRTCTest() {
  const unsigned long nowMs = millis();
  printWiFiStatus();
  if (phase == Phase::Connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      // Wait for a fresh NTP response, not merely an existing ESP system time.
      sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
      configTime(0, 0, Config::NTP_SERVER_PRIMARY, Config::NTP_SERVER_SECONDARY);
      phase = Phase::Syncing;
      phaseStartedMs = nowMs;
      Serial.println("RTC: Wi-Fi connected; requesting NTP time...");
      showStatus("Wi-Fi connected.\nSyncing NTP...");
    } else if (nowMs - phaseStartedMs >= Config::RTC_WIFI_TIMEOUT_MS) {
      finishSync("Wi-Fi timed out; RTC time was not changed");
    }
    return;
  }

  if (phase == Phase::Syncing) {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      const time_t epoch = time(nullptr);
      struct tm utc;
      gmtime_r(&epoch, &utc);
      if (utc.tm_year < 120 || utc.tm_year > 199) {
        finishSync("NTP date outside 2020-2099; RTC time was not changed");
        return;
      }
      if (!rtcResponds()) {
        finishSync("RTC disconnected; cannot save NTP time");
        return;
      }
      // Store UTC; timezone conversion belongs in the display layer.
      rtc.adjust(DateTime(static_cast<uint32_t>(epoch)));
      const DateTime saved = rtc.now();
      const int64_t difference = static_cast<int64_t>(saved.unixtime()) - epoch;
      if (rtc.lostPower() || !saved.isValid() || difference < 0 || difference > 2) {
        finishSync("RTC write verification failed; time may be invalid");
      } else {
        ntpSynced = true;
        finishSync("NTP saved to DS3231 (UTC); RTC now keeps time");
      }
    } else if (nowMs - phaseStartedMs >= Config::RTC_NTP_TIMEOUT_MS) {
      finishSync("NTP timed out; RTC time was not changed");
    }
    return;
  }

  if (nowMs - lastPrintMs < Config::RTC_PRINT_INTERVAL_MS) return;
  lastPrintMs = nowMs;
  if (phase == Phase::Missing) {
    Serial.println(syncResult);
    showStatus(syncResult);
    return;
  }
  if (!rtcResponds()) {
    Serial.println("RTC not responding at 0x68");
    showStatus("RTC not responding\nat 0x68");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("RTC time invalid (lost power). Restore network and restart to sync.");
    showStatus("RTC lost power.\nTime invalid.\nRestart with Wi-Fi.");
    return;
  }
  const DateTime current = rtc.now();
  if (!current.isValid()) {
    Serial.println("RTC returned an invalid date");
    showStatus("RTC date invalid.\nRestart with Wi-Fi.");
    return;
  }
  const DateTime local = current + TimeSpan(Config::RTC_UTC_OFFSET_SECONDS);
  showTime(local);
  Serial.printf("DS3231 %s: %04u-%02u-%02u %02u:%02u:%02u | %s\n",
                Config::RTC_TIMEZONE_LABEL, local.year(), local.month(), local.day(),
                local.hour(), local.minute(), local.second(), syncResult);
}

bool rtcStartupComplete() {
  return phase == Phase::Running || phase == Phase::Missing;
}

uint32_t rtcUtcNow() {
  if (phase != Phase::Running || !rtcResponds() || rtc.lostPower()) return 0;
  const DateTime current = rtc.now();
  return current.isValid() ? current.unixtime() : 0;
}
