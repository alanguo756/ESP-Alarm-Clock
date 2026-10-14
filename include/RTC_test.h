#pragma once

void beginRTCTest();
void updateRTCTest();
void setRTCDisplayVisible(bool visible);

#include <stdint.h>
bool rtcStartupComplete();
uint32_t rtcUtcNow();
