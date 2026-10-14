Weather screens
===============

Select `AppState::RTCTest` in `include/Config.h`. GPIO 6 (button to ground,
internal pull-up) cycles Clock → GY-30 → Weather overview → Today → Tomorrow.
The clock still performs its NTP startup sync once and then uses the DS3231.

Weather is for Vancouver, in Celsius and fixed PDT (UTC−7). Coordinates,
refresh/retry intervals, and page duration are in `include/Config.h`.
If changing the clock timezone, also update `WEATHER_TIMEZONE_ENCODED`.

The overview shows current conditions at top left and today's low/high at top
right. Sunrise and sunset icons sit above their exact times. Below are temperature
bars with time, temperature, and weather labels for sunrise, sunrise +3 hours,
and so on, ending at sunset. The final interval can be shorter than three hours.
The shared bar scale includes zero, with negative temperatures below the baseline.
Three entries fit per page; pages rotate automatically every five seconds.

Today starts at the minute when you open the screen and advances in one-hour
steps until local midnight. Tomorrow covers 00:00 through 23:00. Four entries fit
per page and auto-advance; the button always changes screens. Exact-minute
temperatures are interpolated from hourly forecasts; weather codes use the
nearest hourly sample. These are forecasts, not exact observations at sunrise.

Short weather labels: Clear, Fair, Partly (partly cloudy), Cloud, Fog,
Driz (drizzle), IceDz (freezing drizzle), Rain, IceRn (freezing rain), Snow,
Shwrs (rain showers), SnShw (snow showers), Storm, Hail, or ? (unknown).

After RTC startup completes, a background task fetches weather using verified
HTTPS. It reconnects every 30 minutes, then turns Wi-Fi off after the request.
Failures retry after five minutes. Button handling and the clock continue during
downloads. Cached data stays available on errors, labeled STALE; current conditions
older than one hour are also labeled STALE. Cache is in RAM and resets at reboot.
The RTC needs valid UTC for TLS validation. Errors are shown on the weather screen
and Serial Monitor; Wi-Fi credentials never go to the weather service.

Weather data: [Open-Meteo](https://open-meteo.com/), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
No API key is required for the non-commercial API. See the
[API documentation](https://open-meteo.com/en/docs) for forecast variables.

Firmware build and host parsing/timing checks were run. OLED appearance, network
access from ubcIoT, and button behavior still need verification on the ESP32.
