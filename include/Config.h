#pragma once
#include <stddef.h>
#include <stdint.h>

// Edit settings here, then rebuild and upload.
enum class AppState {
  Off,
  OLEDTest,
  I2CScan,
  GY30Test
};

// Choose one state here, then upload:
// Off      = disable tests and clear the OLED
// OLEDTest = text screens controlled by the GPIO 6 button
// I2CScan  = print device addresses to Serial Monitor
// GY30Test = show light sensor readings on the OLED
constexpr AppState APP_STATE = AppState::I2CScan;

namespace Config {
// Shared wiring and I2C devices.
constexpr int SDA_PIN = 13;
constexpr int SCL_PIN = 14;
constexpr uint32_t I2C_CLOCK_HZ = 100000;
constexpr uint16_t I2C_TIMEOUT_MS = 25;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t SENSOR_ADDRESS = 0x23;

// Display and serial settings. Match monitor_speed in platformio.ini if changed.
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr unsigned long SERIAL_BAUD = 115200;
constexpr unsigned long IDLE_DELAY_MS = 10;

// OLED text/button test. Connect the button between BUTTON_PIN and GND.
constexpr int BUTTON_PIN = 6;
constexpr unsigned long DEBOUNCE_MS = 30;
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

static_assert(FULL_BRIGHTNESS_LUX > 0, "Full brightness lux must be positive");
static_assert(UPDATE_MS >= 180, "Allow at least 180 ms for sensor conversion");
static_assert(MAX_TEXT_SIZE > 0, "Text size must be at least 1");
static_assert(SCAN_START_DELAY_MS <= SCAN_INTERVAL_MS,
              "Scanner startup delay must not exceed scan interval");
} // namespace Config
