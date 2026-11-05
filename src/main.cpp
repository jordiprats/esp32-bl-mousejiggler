#include <Arduino.h>
#include <BleMouse.h>
#include <esp_sleep.h>
#include <esp_system.h>
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
RTC_DATA_ATTR uint8_t       rtcBatteryLevel      = 0;
RTC_DATA_ATTR unsigned long rtcLastBatteryDrop   = 0;
RTC_DATA_ATTR bool          rtcBatteryInit       = false;
RTC_DATA_ATTR unsigned long rtcReconnectDelay    = INIT_RECONNECT_DELAY_MS;
RTC_DATA_ATTR uint64_t      rtcDeepSleepUs       = DEEP_SLEEP_US;
RTC_DATA_ATTR unsigned long rtcFirstDropInterval = 0;
RTC_DATA_ATTR unsigned long rtcLastFluctuation   = 0;

// Globals
BleMouse       bleMouse(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);
BatterySimulator battery;
TimingGenerator  timing;

unsigned long lastConnectedTime = 0;

// Helper: add +/-pct% random jitter to a signed value, clamp to >= 0
static int64_t jitterSigned(int64_t value, int pct) {
  int64_t jitter = value * pct / 100;
  if (jitter > 0) {
    value += ((int64_t)(esp_random() % (2 * (uint32_t)jitter + 1)) - jitter);
  }
  return value < 0 ? 0 : value;
}

void setup() {
  // Lower CPU frequency
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

  // Battery persistence (first-drop interval is restored from RTC inside init)
  battery.init(rtcBatteryLevel, rtcBatteryInit, rtcFirstDropInterval);
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

  // Restore fluctuation timer
  if (rtcLastFluctuation > 0) {
    battery.setLastFluctuation(rtcLastFluctuation);
  }

  // On cold boot (not deep-sleep wakeup), wait 10 min before first connect attempt
  // to give the PC time to boot after power restore.
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
    rtcReconnectDelay = COLD_BOOT_DELAY_MS;
  }

  // Start BLE advertising
  bleMouse.begin();
}

void loop() {
  unsigned long now = millis();

  if (bleMouse.isConnected()) {
    lastConnectedTime = now;
    rtcReconnectDelay = INIT_RECONNECT_DELAY_MS;
    rtcDeepSleepUs    = DEEP_SLEEP_US;

    // Update battery (drain + fluctuations) – temp influences drain rate
    battery.update(now, temperatureRead());
    uint8_t lvl = battery.level();
    bleMouse.setBatteryLevel(lvl);
    rtcBatteryLevel      = lvl;
    rtcLastBatteryDrop    = battery.lastDrop();
    rtcFirstDropInterval  = battery.firstDropInterval();
    rtcLastFluctuation    = battery.lastFluctuation();

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
    // Disconnected – keep battery draining even when not connected
    battery.update(now, temperatureRead());
    uint8_t lvl = battery.level();
    bleMouse.setBatteryLevel(lvl);
    rtcBatteryLevel      = lvl;
    rtcLastBatteryDrop    = battery.lastDrop();
    rtcFirstDropInterval  = battery.firstDropInterval();
    rtcLastFluctuation    = battery.lastFluctuation();

    if (lastConnectedTime == 0) {
      lastConnectedTime = now;
    }

    if (now - lastConnectedTime >= DISCONNECT_TIMEOUT) {
      // Flash LED if a GPIO is configured
      if (kLedGpio >= 0) {
        pinMode(kLedGpio, OUTPUT);
        for (int i = 0; i < 3; ++i) {
          digitalWrite(kLedGpio, HIGH);
          delay(500);
          digitalWrite(kLedGpio, LOW);
          delay(500);
        }
      }
      // Increase deep-sleep duration by 5% each cycle, cap at 3 min
      rtcDeepSleepUs = (rtcDeepSleepUs * 105UL) / 100UL;
      if (rtcDeepSleepUs > MAX_DEEP_SLEEP_US) rtcDeepSleepUs = MAX_DEEP_SLEEP_US;
      // Jitter by +/-10%
      uint64_t sleepUs = (uint64_t)jitterSigned((int64_t)rtcDeepSleepUs, 10);
      esp_sleep_enable_timer_wakeup(sleepUs);
      esp_deep_sleep_start();
    }

    // Jitter the reconnect delay by +/-10%
    unsigned long actualDelay = (unsigned long)jitterSigned((int64_t)rtcReconnectDelay, 10);
    delay(actualDelay);
    // Increase backoff by 3%, cap at 3 minutes
    rtcReconnectDelay = (rtcReconnectDelay * 103UL) / 100UL;
    if (rtcReconnectDelay > MAX_RECONNECT_DELAY_MS) rtcReconnectDelay = MAX_RECONNECT_DELAY_MS;
  }
}
