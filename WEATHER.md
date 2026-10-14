Weather screens
===============

Select `AppState::Main` in `include/Config.h` for the clock/weather interface
with background light sensing. The GY-30 adjusts OLED contrast linearly from
a minimum of 30 in darkness to 255 at 140 lux, without displaying a sensor screen.
`AppState::RTCTest` keeps the same screens with fixed brightness. GPIO 6 (button to ground,
internal pull-up) cycles Clock → Weather overview → Today → Tomorrow.
The clock still performs its NTP startup sync once and then uses the DS3231.

Weather is for Vancouver, in Celsius and fixed PDT (UTC−7). Coordinates,
refresh/retry intervals, and button pins are in `include/Config.h`.
If changing the clock timezone, also update `WEATHER_TIMEZONE_ENCODED`.

The overview shows current conditions at top left and today's low/high at top
right. Sunrise and sunset icons sit above their exact times, with weather and
temperature labels only at those two endpoints. Underneath, 24 temperature bars
cover the full day from 00:00 through 23:00. The bars share a zero-inclusive scale;
negative temperatures extend below the baseline. This overview no longer pages.

Today starts at the minute when you open the screen and advances in one-hour
steps until local midnight. Tomorrow covers 00:00 through 23:00. Four entries fit
per page. Press the GPIO 16 button (to GND, internal pull-up) to advance one page,
wrapping to the first page after the last. Holding it does not repeat. GPIO 6
changes screens and resets paging to page 1. Pages do not advance automatically. Exact-minute
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

Main clock alarm
----------------

On the Main time screen, GPIO 16 opens/saves a 24-hour alarm editor. GPIO 1
increments the underlined digit and GPIO 6 selects the next digit. Outside the
editor, GPIO 6 resumes normal screen switching; GPIO 16 still pages weather.
The alarm remains active on every screen and returns to the clock when ringing.
Press GPIO 16 to silence it. Alarms are one-shot and stored in RAM, so resets
clear them. Automatic brightness remains active during editing and ringing.
