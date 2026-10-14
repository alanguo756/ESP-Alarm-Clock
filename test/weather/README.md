Weather checks
==============

Run from the project root after `pio run` installs ArduinoJson:

```sh
c++ -std=c++11 -Iinclude -I.pio/libdeps/esp32-s3-devkitm-1/ArduinoJson/src test/weather/check.cpp -o /tmp/alarmclock-weather-check
/tmp/alarmclock-weather-check test/weather/vancouver.json
```

The fixture is a Vancouver Open-Meteo forecast downloaded on 2026-10-12.
Checks cover real JSON parsing, null fields, negative temperatures, forecast
coverage, interpolation, three-hour spacing, and the shorter sunset endpoint.

Data source: [Open-Meteo](https://open-meteo.com/) ([API documentation](https://open-meteo.com/en/docs)),
licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
