#include <Arduino.h>
#include <BleMouse.h>
#include <esp_sleep.h>
#include <config.h>
#include <battery.h>
#include <timing.h>

// Optional local override for BLE device identity
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

// RTC-retained variables
RTC_DATA_ATTR uint8_t      rtcBatteryLevel    = 0;
RTC_DATA_ATTR unsigned long rtcLastBatteryDrop = 0;
RTC_DATA_ATTR bool         rtcBatteryInit     = false;
RTC_DATA_ATTR unsigned long rtcReconnectDelay = 5000UL;

// Globals
BleMouse       bleMouse(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);
BatterySimulator battery;
TimingGenerator  timing;

unsigned long lastConnectedTime = 0;

void setup() {
  // Lower CPU frequency
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

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

  // Start BLE advertising
  bleMouse.begin();
}

void loop() {
  unsigned long now = millis();

  if (bleMouse.isConnected()) {
    lastConnectedTime = now;
    rtcReconnectDelay = 5000UL;

    // Update battery (drain + fluctuations) – temp influences drain rate
    battery.update(now, temperatureRead());
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
      lastConnectedTime = millis();
    }

  } else {
    // Disconnected
    if (lastConnectedTime == 0) {
      lastConnectedTime = now;
    }

    if (now - lastConnectedTime >= DISCONNECT_TIMEOUT) {
      // Flash LED if built-in LED is available
#ifdef LED_BUILTIN
      pinMode(LED_BUILTIN, OUTPUT);
      for (int i = 0; i < 3; ++i) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
      }
#endif
      esp_sleep_enable_timer_wakeup(DEEP_SLEEP_US);
      esp_deep_sleep_start();
    }

    delay(rtcReconnectDelay);
    // Increase backoff by 3%, cap at 3 minutes
    rtcReconnectDelay = (rtcReconnectDelay * 103UL) / 100UL;
    if (rtcReconnectDelay > 180000UL) rtcReconnectDelay = 180000UL;
  }
}
