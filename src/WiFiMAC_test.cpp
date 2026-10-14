#include <Arduino.h>
#include <esp_mac.h>
#include "Config.h"
#include "WiFiMAC_test.h"

namespace {
unsigned long lastPrintMs = 0;
}

void beginWiFiMACTest() {
  lastPrintMs = millis() - Config::MAC_PRINT_INTERVAL_MS;
}

void updateWiFiMACTest() {
  if (millis() - lastPrintMs < Config::MAC_PRINT_INTERVAL_MS) return;
  lastPrintMs = millis();
  uint8_t mac[6];
  // Read the station address used when connecting to a Wi-Fi router.
  // No Wi-Fi connection or credentials are needed.
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
    Serial.println("Failed to read Wi-Fi station MAC address");
    return;
  }
  Serial.printf("Wi-Fi station MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
