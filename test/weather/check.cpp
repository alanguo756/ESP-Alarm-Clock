#include <cassert>
#include <fstream>
#include <iostream>
#include "WeatherParse.h"

int main(int argc, char** argv) {
  assert(argc == 2);
  std::ifstream file(argv[1]);
  JsonDocument doc;
  assert(!deserializeJson(doc, file));
  WeatherData data{};
  assert(parseWeather(doc, data));
  assert(data.count == 72);
  uint32_t times[10];
  const size_t count = daylightTimes(data.days[0], times, 10);
  assert(count >= 2 && times[0] == data.days[0].sunrise);
  assert(times[count - 1] == data.days[0].sunset);
  for (size_t i = 1; i + 1 < count; ++i) assert(times[i] - times[i - 1] == 10800);
  assert(times[count - 1] - times[count - 2] <= 10800);
  WeatherHour sample{};
  for (size_t i = 0; i < count; ++i) assert(weatherAt(data, times[i], sample));
  assert(!weatherAt(data, data.hours[0].time - 1, sample));
  assert(!weatherAt(data, data.hours[71].time + 1, sample));
  assert(weatherAt(data, data.hours[0].time + 1800, sample));
  assert(fabs(sample.temperature - (data.hours[0].temperature + data.hours[1].temperature) / 2) < 0.001);
  // All of tomorrow's 24 hourly entries must have forecast coverage.
  for (unsigned i = 0; i < 24; ++i) assert(weatherAt(data, data.days[1].start + i * 3600, sample));
  WeatherDay noSun{0, 0, 0, 0, 0};
  assert(daylightTimes(noSun, times, 10) == 0);
  doc["hourly"]["temperature_2m"][2] = nullptr;
  assert(!parseWeather(doc, data));
  doc["hourly"]["temperature_2m"][2] = -5;
  assert(parseWeather(doc, data));
  doc["daily"]["sunset"][0] = 0;
  assert(!parseWeather(doc, data));
  std::cout << "Weather parsing, missing data, interpolation, sunrise/sunset and tomorrow checks passed.\n";
}
