// Local override – must precede all other includes so config.h sees the overrides
#ifdef __has_include
#if __has_include("config.local.h")
#include "config.local.h"
#endif
#endif

#include <Arduino.h>
#include <BleMouse.h>
#include <esp_system.h>
#include <battery.h>
#include <timing.h>

#if FAKE_SSID_ENABLED
#include <WiFi.h>
#endif

#include <config.h>

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME  "ESP32 Mouse"
#endif
#ifndef BLE_MANUFACTURER
#define BLE_MANUFACTURER "Espressif"
#endif

// Backoff state (persists across loop iterations, reset on connect)
unsigned long reconnectDelay = INIT_RECONNECT_DELAY_MS;



// Globals
BleMouse         bleMouse(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);
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

  // Battery – start with a fresh random level on each power-up
  battery.init(0, false);
  bleMouse.batteryLevel = battery.level();

  // Start BLE advertising
  bleMouse.begin();

#if FAKE_SSID_ENABLED
  // Beacon a single fake SSID – pure AP, no web UI
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(FAKE_SSID);
#endif
}

void loop() {
  unsigned long now = millis();

  if (bleMouse.isConnected()) {
    lastConnectedTime = now;
    reconnectDelay = INIT_RECONNECT_DELAY_MS;

    // Update battery (drain + fluctuations) – temp influences drain rate
    battery.update(now, temperatureRead());
    bleMouse.setBatteryLevel(battery.level());

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
    bleMouse.setBatteryLevel(battery.level());

    if (lastConnectedTime == 0) {
      lastConnectedTime = now;
    }

    // Wait with jittered exponential backoff, then retry advertising
    unsigned long actualDelay = (unsigned long)jitterSigned((int64_t)reconnectDelay, 10);
    delay(actualDelay);

    // Increase backoff by 3%, cap at 3 minutes
    reconnectDelay = (reconnectDelay * 103UL) / 100UL;
    if (reconnectDelay > MAX_RECONNECT_DELAY_MS) reconnectDelay = MAX_RECONNECT_DELAY_MS;
  }
}
