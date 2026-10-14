#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "WiFiSecrets.h"
#include "WiFi_test.h"

namespace {
unsigned long lastAttemptMs = 0;
unsigned long lastStatusMs = 0;
bool wasConnected = false;

void connectWiFi() {
  Serial.printf("Connecting to %s...\n", WiFiSecrets::SSID);
  WiFi.begin(WiFiSecrets::SSID, WiFiSecrets::PASSWORD);
  lastAttemptMs = millis();
}
}

void beginWiFiTest() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  wasConnected = false;
  lastStatusMs = millis() - Config::WIFI_STATUS_INTERVAL_MS;
  connectWiFi();
}

void updateWiFiTest() {
  const unsigned long now = millis();
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected ||
      now - lastStatusMs >= Config::WIFI_STATUS_INTERVAL_MS) {
    lastStatusMs = now;
    if (connected) {
      Serial.printf("Wi-Fi connected: %s | IP: %s | MAC: %s | RSSI: %d dBm\n",
                    WiFiSecrets::SSID, WiFi.localIP().toString().c_str(),
                    WiFi.macAddress().c_str(), WiFi.RSSI());
    } else {
      Serial.printf("Wi-Fi waiting for %s | status: %d | MAC: %s\n",
                    WiFiSecrets::SSID, static_cast<int>(WiFi.status()),
                    WiFi.macAddress().c_str());
    }
  }
  if (wasConnected && !connected) lastAttemptMs = now;
  wasConnected = connected;

  // Retry failed initial connections as well as connections lost later.
  if (!connected && now - lastAttemptMs >= Config::WIFI_RETRY_INTERVAL_MS) {
    WiFi.disconnect();
    connectWiFi();
  }
}
