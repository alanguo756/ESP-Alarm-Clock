#include <Arduino.h>
#include "Config.h"
#include "Buzzer_test.h"

namespace {
bool buzzing = false;
unsigned long startedMs = 0;
}

void beginBuzzerTest() {
  pinMode(Config::BUZZER_PIN, OUTPUT);
  digitalWrite(Config::BUZZER_PIN, LOW);
  tone(Config::BUZZER_PIN, Config::BUZZER_FREQUENCY_HZ);
  startedMs = millis();
  buzzing = true;
}

void updateBuzzerTest() {
  const unsigned long now = millis();
  if (now - startedMs >= Config::BUZZER_DURATION_MS) {
    buzzing = !buzzing;
    if (buzzing) {
      tone(Config::BUZZER_PIN, Config::BUZZER_FREQUENCY_HZ);
    } else {
      noTone(Config::BUZZER_PIN);
      digitalWrite(Config::BUZZER_PIN, LOW);
    }
    startedMs = now;
  }
}
