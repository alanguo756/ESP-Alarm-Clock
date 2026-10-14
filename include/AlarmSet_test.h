#pragma once
void beginAlarmSetTest();
void updateAlarmSetTest();

// Shared alarm controller for the Main clock; does not restart RTC/Wi-Fi.
void beginAlarmController();
void updateAlarmController(bool clockVisible, bool savePressed, bool nextPressed, bool incrementPressed);
bool alarmOverlayActive();
bool alarmIsRinging();
