#pragma once
#include <stdint.h>

// 0: overview, 1: today hourly, 2: tomorrow hourly, -1: hidden.
void selectWeatherScreen(int screen);
void nextWeatherPage();
void updateWeatherScreens(bool startupComplete, uint32_t rtcUtc);
