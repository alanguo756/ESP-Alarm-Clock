#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>

struct WeatherHour { uint32_t time; float temperature; int code; };
struct WeatherDay { uint32_t start, sunrise, sunset; float low, high; };
struct WeatherData {
  uint32_t currentTime;
  float temperature;
  int code;
  size_t count;
  WeatherHour hours[72];
  WeatherDay days[3];
};

// Interpolate temperature at exact sunrise/current-minute timestamps.
// Weather categories use the nearest hourly forecast, not interpolation.
inline bool weatherAt(const WeatherData& data, uint32_t when, WeatherHour& out) {
  if (!data.count || when < data.hours[0].time || when > data.hours[data.count - 1].time) return false;
  for (size_t i = 0; i < data.count; ++i) {
    if (data.hours[i].time == when) { out = data.hours[i]; return true; }
    if (data.hours[i].time > when && i) {
      const auto& a = data.hours[i - 1];
      const auto& b = data.hours[i];
      const float fraction = float(when - a.time) / (b.time - a.time);
      out = {when, a.temperature + fraction * (b.temperature - a.temperature),
             fraction < 0.5f ? a.code : b.code};
      return true;
    }
  }
  return false;
}

inline size_t daylightTimes(const WeatherDay& day, uint32_t* times, size_t capacity) {
  if (!day.sunrise || day.sunset <= day.sunrise || capacity < 2) return 0;
  size_t count = 0;
  for (uint32_t t = day.sunrise; t < day.sunset && count < capacity - 1; t += 10800) times[count++] = t;
  times[count++] = day.sunset;
  return count;
}

inline const char* weatherLabel(int code) {
  switch (code) {
    case 0: return "Clear";
    case 1: return "Fair";
    case 2: return "Partly";
    case 3: return "Cloud";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Driz";
    case 56: case 57: return "IceDz";
    case 61: case 63: case 65: return "Rain";
    case 66: case 67: return "IceRn";
    case 71: case 73: case 75: case 77: return "Snow";
    case 80: case 81: case 82: return "Shwrs";
    case 85: case 86: return "SnShw";
    case 95: return "Storm";
    case 96: case 99: return "Hail";
    default: return "?";
  }
}
