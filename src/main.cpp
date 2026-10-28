#include <Arduino.h>
#include <BleMouse.h>
#include <esp_sleep.h>
#include <config.h>
#include <battery.h>
#include <timing.h>

// Optional local override for BLE device identity
// Use a config.local.h in src/ to set realistic names
#ifdef __has_include
#if __has_include("config.local.h")
#include "config.local.h"
#endif
#endif

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME  "ESP32 Mouse"
#endif
#ifndef BLE_MANUFACTURER
#define BLE_MANUFACTURER "Espressif"
#endif

// RTC-retained variables (survive deep sleep)
RTC_DATA_ATTR uint8_t      rtcBatteryLevel    = 0;
RTC_DATA_ATTR unsigned long rtcLastBatteryDrop = 0;
RTC_DATA_ATTR bool         rtcBatteryInit     = false;
RTC_DATA_ATTR unsigned int rtcMoveIndex       = 0;

// Globals
BleMouse       bleMouse(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);
BatterySimulator battery;
TimingGenerator  timing;

unsigned long lastConnectedTime = 0;

void setup() {
  // Battery persistence
  battery.init(rtcBatteryLevel, rtcBatteryInit);
  rtcBatteryLevel = battery.level();
  rtcBatteryInit  = true;
  bleMouse.batteryLevel = battery.level();

  // Restore battery-drop timer
  unsigned long now = millis();
  battery.setLastDrop(now);
  if (now < rtcLastBatteryDrop) {
    rtcLastBatteryDrop = now;
  } else {
    battery.setLastDrop(rtcLastBatteryDrop);
  }

  // Restore move index
  timing.reset();
  rtcMoveIndex = 0;

  // Start BLE advertising
  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    unsigned long now = millis();
    lastConnectedTime = now;

    // Update battery (drain + fluctuations) 
    battery.update(now);
    uint8_t lvl = battery.level();
    bleMouse.setBatteryLevel(lvl);
    rtcBatteryLevel = lvl;

    // Move 
    Move m = timing.nextMove();
    bleMouse.move(m.dx, m.dy);
    delay(timing.moveDelay());

    // Idle pause (random, after any move)
    if (timing.maybeIdlePause()) {
      delay(timing.idlePauseDuration());
      if (bleMouse.isConnected()) {
        lastConnectedTime = millis();
      }
    }

  } else {
    // Disconnected 
    if (lastConnectedTime == 0) {
      lastConnectedTime = millis();
    }

    if (millis() - lastConnectedTime >= DISCONNECT_TIMEOUT) {
      esp_sleep_enable_timer_wakeup(DEEP_SLEEP_US);
      esp_deep_sleep_start();
    }

    delay(5000);
  }
}
