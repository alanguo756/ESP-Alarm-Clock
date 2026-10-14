#pragma once
#include <stddef.h>
#include <stdint.h>

// Edit settings here, then rebuild and upload.
enum class AppState {
  Off,
  OLEDTest,
  I2CScan,
  GY30Test,
  WiFiMACTest,
  WiFiTest,
  RTCTest
};

// Choose one state here, then upload:
// Off      = disable tests and clear the OLED
// OLEDTest = text screens controlled by the GPIO 6 button
// I2CScan  = print device addresses to Serial Monitor
// GY30Test = show light sensor readings on the OLED
// WiFiMACTest = print the Wi-Fi station MAC address
// WiFiTest = connect using WiFiSecrets.h and print connection status/IP
// RTCTest = clock, GY-30, weather overview, today hourly, tomorrow hourly
constexpr AppState APP_STATE = AppState::RTCTest;

namespace Config {
// Shared wiring and I2C devices.
constexpr int SDA_PIN = 13;
constexpr int SCL_PIN = 14;
constexpr uint32_t I2C_CLOCK_HZ = 100000;
constexpr uint16_t I2C_TIMEOUT_MS = 25;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t SENSOR_ADDRESS = 0x23;
constexpr uint8_t TIME_ADDRESS = 0x68;

// Display and serial settings. Match monitor_speed in platformio.ini if changed.
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr unsigned long SERIAL_BAUD = 115200;
constexpr unsigned long IDLE_DELAY_MS = 10;

// OLED text/button test. Connect the button between BUTTON_PIN and GND.
constexpr int BUTTON_PIN = 6;
constexpr int PAGE_BUTTON_PIN = 16; // Button to GND; cycles pages within a screen.
constexpr unsigned long DEBOUNCE_MS = 30;
// RTCTest cycles between the clock and GY-30 with this same button.
constexpr uint8_t CLOCK_BRIGHTNESS = 207;
constexpr uint8_t MAX_TEXT_SIZE = 2;
// Edit these strings to change the text on each screen.
constexpr const char* SCREEN_TEXT[] = {
  "How to describe Susie in 3 words",
  "Nipply",
  "Bipply",
  "and Jipply"
};

// Cycle through the first SCREEN_COUNT entries. Add text above for more screens.
constexpr size_t SCREEN_COUNT = 4;
static_assert(SCREEN_COUNT > 0, "At least one screen is required");
static_assert(SCREEN_COUNT <= sizeof(SCREEN_TEXT) / sizeof(SCREEN_TEXT[0]),
              "Add a text entry for every screen");

// I2C scanner timing.
constexpr unsigned long SCAN_INTERVAL_MS = 5000;
constexpr unsigned long SCAN_START_DELAY_MS = 1500;

// Light sensor: keep UPDATE_MS >= 180 ms for its conversion time.
constexpr unsigned long UPDATE_MS = 500;
// Linear contrast: 0 lux -> 0 (dimmest), FULL_BRIGHTNESS_LUX -> 255.
constexpr float FULL_BRIGHTNESS_LUX = 140.0f;

// Mac address printing timer.
constexpr unsigned long MAC_PRINT_INTERVAL_MS = 3000;
// Wi-Fi connection timing. Credentials are in the Git-ignored WiFiSecrets.h.
constexpr unsigned long WIFI_STATUS_INTERVAL_MS = 5000;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 30000;

// DS3231 startup synchronization (stored in UTC; displayed with the offset below).
constexpr const char* NTP_SERVER_PRIMARY = "pool.ntp.org";
constexpr const char* NTP_SERVER_SECONDARY = "time.google.com";
constexpr unsigned long RTC_WIFI_TIMEOUT_MS = 30000;
constexpr unsigned long RTC_NTP_TIMEOUT_MS = 20000;
constexpr unsigned long RTC_PRINT_INTERVAL_MS = 1000;
// Fixed PDT (UTC-7); does not automatically switch to PST in winter.
constexpr int32_t RTC_UTC_OFFSET_SECONDS = -7 * 60 * 60;
constexpr const char* RTC_TIMEZONE_LABEL = "PDT";

// Vancouver weather, Open-Meteo. Keep API timezone consistent with RTC display.
constexpr float WEATHER_LATITUDE = 49.2827f;
constexpr float WEATHER_LONGITUDE = -123.1207f;
constexpr const char* WEATHER_TIMEZONE_ENCODED = "Etc%2FGMT%2B7"; // fixed PDT
constexpr unsigned long WEATHER_REFRESH_MS = 30UL * 60 * 1000;
constexpr unsigned long WEATHER_RETRY_MS = 5UL * 60 * 1000;
constexpr unsigned long WEATHER_WIFI_TIMEOUT_MS = 30000;
constexpr uint16_t WEATHER_HTTP_TIMEOUT_MS = 10000;
constexpr uint32_t WEATHER_TASK_STACK = 16384;
static_assert(WEATHER_LATITUDE >= -90 && WEATHER_LATITUDE <= 90,
              "Weather latitude must be between -90 and 90");
static_assert(WEATHER_LONGITUDE >= -180 && WEATHER_LONGITUDE <= 180,
              "Weather longitude must be between -180 and 180");

static_assert(FULL_BRIGHTNESS_LUX > 0, "Full brightness lux must be positive");
static_assert(UPDATE_MS >= 180, "Allow at least 180 ms for sensor conversion");
static_assert(MAX_TEXT_SIZE > 0, "Text size must be at least 1");
static_assert(SCAN_START_DELAY_MS <= SCAN_INTERVAL_MS,
              "Scanner startup delay must not exceed scan interval");
} // namespace Config
