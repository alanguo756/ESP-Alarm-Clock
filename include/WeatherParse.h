#pragma once
#include <ArduinoJson.h>
#include "WeatherData.h"

inline bool parseWeather(JsonDocument& doc, WeatherData& out) {
  if (doc["error"] == true) return false;
  JsonObjectConst current = doc["current"];
  if (!current["time"].is<uint32_t>() || !current["temperature_2m"].is<float>() ||
      !current["weather_code"].is<int>()) return false;
  out.currentTime = current["time"];
  out.temperature = current["temperature_2m"];
  out.code = current["weather_code"];
  if (!isfinite(out.temperature)) return false;
  JsonObjectConst hours = doc["hourly"];
  out.count = hours["time"].size();
  if (out.count < 48 || out.count > 72 || hours["temperature_2m"].size() != out.count ||
      hours["weather_code"].size() != out.count) return false;
  for (size_t i = 0; i < out.count; ++i) {
    if (!hours["time"][i].is<uint32_t>() || !hours["temperature_2m"][i].is<float>() ||
        !hours["weather_code"][i].is<int>()) return false;
    out.hours[i] = {hours["time"][i], hours["temperature_2m"][i], hours["weather_code"][i]};
    if (!isfinite(out.hours[i].temperature) ||
        (i && out.hours[i].time != out.hours[i - 1].time + 3600)) return false;
  }
  JsonObjectConst daily = doc["daily"];
  for (int i = 0; i < 3; ++i) {
    if (!daily["time"][i].is<uint32_t>() || !daily["sunrise"][i].is<uint32_t>() ||
        !daily["sunset"][i].is<uint32_t>() || !daily["temperature_2m_min"][i].is<float>() ||
        !daily["temperature_2m_max"][i].is<float>()) return false;
    out.days[i] = {daily["time"][i], daily["sunrise"][i], daily["sunset"][i],
                   daily["temperature_2m_min"][i], daily["temperature_2m_max"][i]};
    const auto& day = out.days[i];
    if (!isfinite(day.low) || !isfinite(day.high) || day.low > day.high ||
        day.sunset <= day.sunrise || (i && day.start != out.days[i - 1].start + 86400)) return false;
  }
  return true;
}
