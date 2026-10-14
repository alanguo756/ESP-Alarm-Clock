#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <sys/time.h>
#include "Config.h"
#include "WiFiSecrets.h"
#include "WeatherCA.h"
#include "WeatherParse.h"
#include "WeatherScreens.h"
#include "AutoBrightness.h"

namespace {
struct FetchResult { bool ok; WeatherData data; char error[64]; };
QueueHandle_t resultQueue = nullptr;
WeatherData forecast{};
bool haveData = false, fetching = false, attempted = false, lastFetchOK = false;
unsigned long lastAttempt = 0, receivedMs = 0, lastRender = 0;
unsigned currentPage = 0;
unsigned pageCount = 1;
uint32_t screenAnchor = 0;
int selected = -1;
char errorText[64] = "Waiting for clock sync";
Adafruit_SSD1306 display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, -1);
bool displayReady = false;

bool fetchForecast(FetchResult& result) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.begin(WiFiSecrets::SSID, WiFiSecrets::PASSWORD);
  Serial.println("[Weather] Wi-Fi connecting");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= Config::WEATHER_WIFI_TIMEOUT_MS) {
      snprintf(result.error, sizeof(result.error), "Weather Wi-Fi timeout");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  Serial.println("[Weather] Wi-Fi connected; fetching Open-Meteo");
  WiFiClientSecure client;
  client.setCACert(WEATHER_ROOT_CA);
  client.setHandshakeTimeout(10);
  HTTPClient http;
  http.setConnectTimeout(Config::WEATHER_HTTP_TIMEOUT_MS);
  http.setTimeout(Config::WEATHER_HTTP_TIMEOUT_MS);
  http.useHTTP10(true); // Avoid chunk framing when parsing the response stream.
  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(Config::WEATHER_LATITUDE, 4);
  url += "&longitude="; url += String(Config::WEATHER_LONGITUDE, 4);
  url += "&current=temperature_2m,weather_code&hourly=temperature_2m,weather_code";
  url += "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset";
  url += "&timeformat=unixtime&forecast_days=3&timezone=";
  url += Config::WEATHER_TIMEZONE_ENCODED;
  if (!http.begin(client, url)) {
    snprintf(result.error, sizeof(result.error), "HTTPS setup failed");
    return false;
  }
  const int status = http.GET();
  bool valid = false;
  if (status == HTTP_CODE_OK) {
    JsonDocument doc;
    const auto error = deserializeJson(doc, http.getStream());
    valid = !error && parseWeather(doc, result.data);
    if (!valid) snprintf(result.error, sizeof(result.error), "Invalid weather response");
  } else {
    snprintf(result.error, sizeof(result.error), "Weather HTTP error %d", status);
  }
  http.end();
  return valid;
}

void fetchTask(void*) {
  FetchResult result{};
  result.ok = fetchForecast(result);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.printf("[Weather] %s; Wi-Fi %s\n", result.ok ? "Forecast downloaded" : result.error,
                WiFi.getMode() == WIFI_OFF ? "OFF" : "shutdown failed");
  xQueueSend(resultQueue, &result, portMAX_DELAY);
  vTaskDelete(nullptr);
}

void label(int x, int y, const char* text) {
  display.setCursor(x, y); display.print(text);
}

void formatTime(uint32_t epoch, char* buffer) {
  const DateTime local(epoch + Config::RTC_UTC_OFFSET_SECONDS);
  snprintf(buffer, 6, "%02u:%02u", local.hour(), local.minute());
}

void sunIcon(int x, int y, bool rising) {
  display.drawCircle(x, y + 4, 3, SSD1306_WHITE);
  display.fillRect(x - 4, y + 5, 9, 4, SSD1306_BLACK);
  display.drawFastHLine(x - 6, y + 5, 13, SSD1306_WHITE);
  display.drawFastVLine(x, y - 2, 2, SSD1306_WHITE);
  const int ax = x + 10;
  display.drawFastVLine(ax, y, 6, SSD1306_WHITE);
  const int tip = rising ? y : y + 5;
  const int base = rising ? y + 2 : y + 3;
  display.drawLine(ax, tip, ax - 2, base, SSD1306_WHITE);
  display.drawLine(ax, tip, ax + 2, base, SSD1306_WHITE);
}

int todayIndex(uint32_t now) {
  for (int i = 0; i < 3; ++i) {
    if (now >= forecast.days[i].start && now < forecast.days[i].start + 86400) return i;
  }
  return -1;
}

void overview(int dayIndex) {
  const auto& day = forecast.days[dayIndex];
  char text[32];
  snprintf(text, sizeof(text), "%s %.0fC", weatherLabel(forecast.code), forecast.temperature);
  label(0, 0, text);
  snprintf(text, sizeof(text), "%.0f-%.0fC", day.low, day.high);
  label(max(0, 128 - int(strlen(text)) * 6), 0, text);
  sunIcon(12, 11, true); sunIcon(108, 11, false);
  formatTime(day.sunrise, text); label(0, 19, text);
  formatTime(day.sunset, text); label(98, 19, text);
  // Only sunrise and sunset have condition labels on this overview.
  const uint32_t endpoints[] = {day.sunrise, day.sunset};
  for (int i = 0; i < 2; ++i) {
    WeatherHour hour;
    if (weatherAt(forecast, endpoints[i], hour)) {
      snprintf(text, sizeof(text), "%s %.0fC", weatherLabel(hour.code), hour.temperature);
    } else snprintf(text, sizeof(text), "N/A");
    label(i == 0 ? 0 : 128 - int(strlen(text)) * 6, 28, text);
  }

  // One bar per hour, midnight through 23:00. A shared zero baseline
  // keeps negative temperatures meaningful instead of implying positive heat.
  float low = fminf(0, floorf(day.low));
  float high = fmaxf(1, ceilf(day.high));
  WeatherHour hours[24];
  bool available[24];
  for (int i = 0; i < 24; ++i) {
    available[i] = weatherAt(forecast, day.start + i * 3600, hours[i]);
    if (available[i]) {
      low = fminf(low, floorf(hours[i].temperature));
      high = fmaxf(high, ceilf(hours[i].temperature));
    }
  }
  const int graphTop = 38, graphBottom = 54;
  const int zeroY = graphBottom - lroundf(-low / (high - low) * (graphBottom - graphTop));
  display.drawFastHLine(4, zeroY, 120, SSD1306_WHITE);
  for (int i = 0; i < 24; ++i) {
    if (!available[i]) continue; // Missing forecasts are not zero temperatures.
    const int tempY = constrain(graphBottom - int(lroundf(
        (hours[i].temperature - low) / (high - low) * (graphBottom - graphTop))),
        graphTop, graphBottom);
    display.fillRect(4 + i * 5, min(zeroY, tempY), 4,
                     max(1, abs(tempY - zeroY)), SSD1306_WHITE);
  }
  label(0, 56, "00"); label(30, 56, "06"); label(60, 56, "12");
  label(90, 56, "18"); label(116, 56, "23");

}

void hourly(uint32_t now, int dayIndex, bool tomorrow) {
  char text[32];
  uint32_t start, end;
  if (tomorrow) {
    if (dayIndex + 1 >= 3) { label(0, 20, "Tomorrow unavailable"); return; }
    start = forecast.days[dayIndex + 1].start; end = start + 86400;
  } else {
    if (!screenAnchor || screenAnchor < forecast.days[dayIndex].start) screenAnchor = now - now % 60;
    start = screenAnchor;
    end = forecast.days[dayIndex].start + 86400;
  }
  const unsigned count = (end - start + 3599) / 3600;
  const unsigned pages = max(1U, (count + 3) / 4);
  pageCount = pages;
  currentPage %= pages;
  const unsigned page = currentPage;
  snprintf(text, sizeof(text), "%s %u/%u %s", tomorrow ? "Tomorrow" : "Today", page + 1, pages, Config::RTC_TIMEZONE_LABEL);
  label(0, 0, text);
  for (unsigned row = 0; row < 4 && page * 4 + row < count; ++row) {
    const uint32_t t = start + (page * 4 + row) * 3600;
    char timeText[6]; formatTime(t, timeText);
    WeatherHour hour;
    if (weatherAt(forecast, t, hour)) {
      snprintf(text, sizeof(text), "%s %4.0fC %-5s", timeText, hour.temperature, weatherLabel(hour.code));
    } else snprintf(text, sizeof(text), "%s  unavailable", timeText);
    label(0, 12 + row * 10, text);
  }
  label(0, 56, "Open-Meteo  C");
}
}

void selectWeatherScreen(int screen) {
  selected = screen;
  currentPage = 0; pageCount = 1;
  screenAnchor = 0; lastRender = millis() - 1000;
  if (screen < 0) return;
  if (!displayReady) {
    Wire.beginTransmission(Config::OLED_ADDRESS);
    displayReady = Wire.endTransmission() == 0 &&
        display.begin(SSD1306_SWITCHCAPVCC, Config::OLED_ADDRESS, true, false);
  }
  if (displayReady) {
    display.setTextSize(1); display.setTextWrap(false); display.setTextColor(SSD1306_WHITE);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(currentOLEDBrightness());
  }
}

void nextWeatherPage() {
  if (selected != 1 && selected != 2) return;
  currentPage = (currentPage + 1) % pageCount;
  lastRender = millis() - 1000;
}

void updateWeatherScreens(bool startupComplete, uint32_t rtcUtc) {
  if (!resultQueue) resultQueue = xQueueCreate(1, sizeof(FetchResult));
  FetchResult result;
  if (resultQueue && xQueueReceive(resultQueue, &result, 0) == pdTRUE) {
    fetching = false; lastFetchOK = result.ok;
    if (result.ok) {
      forecast = result.data; haveData = true; receivedMs = millis();
      Serial.println("Weather data: Open-Meteo (CC BY 4.0). Temperatures in Celsius.");
    } else snprintf(errorText, sizeof(errorText), "%s", result.error);
  }
  const unsigned long interval = lastFetchOK ? Config::WEATHER_REFRESH_MS : Config::WEATHER_RETRY_MS;
  if (startupComplete && rtcUtc && !fetching && resultQueue &&
      (!attempted || millis() - lastAttempt >= interval)) {
    // TLS certificate validation needs valid system UTC, including offline RTC boot.
    timeval tv{static_cast<time_t>(rtcUtc), 0}; settimeofday(&tv, nullptr);
    attempted = true; lastAttempt = millis();
    fetching = xTaskCreate(fetchTask, "weather", Config::WEATHER_TASK_STACK, nullptr, 1, nullptr) == pdPASS;
    if (!fetching) { lastFetchOK = false; snprintf(errorText, sizeof(errorText), "Weather task failed"); }
  }
  if (selected < 0 || !displayReady || millis() - lastRender < 250) return;
  lastRender = millis();
  display.clearDisplay();
  pageCount = 1; // Loading, errors, and the overview have only one page.
  if (!haveData) {
    label(0, 0, "Vancouver weather");
    display.setTextWrap(true); display.setCursor(0, 18);
    display.print(fetching ? "Fetching forecast..." : errorText);
    if (startupComplete && !rtcUtc) { display.setCursor(0, 40); display.print("Clock needs NTP sync"); }
    display.setTextWrap(false);
    label(0, 56, "Open-Meteo");
  } else {
    const uint32_t now = rtcUtc ? rtcUtc : forecast.currentTime + (millis() - receivedMs) / 1000;
    const int day = todayIndex(now);
    if (day < 0) label(0, 20, "Forecast expired");
    else if (selected == 0) overview(day);
    else hourly(now, day, selected == 2);
    // Current conditions are cached; visibly mark old data or refresh failures.
    if (!lastFetchOK || now > forecast.currentTime + 3600) label(selected == 0 ? 38 : 92, selected == 0 ? 12 : 56, "STALE");
  }
  display.display();
}
