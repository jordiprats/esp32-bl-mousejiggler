#include <Arduino.h>
#include <BleMouse.h>
#include <esp_sleep.h>
#include <esp_system.h>

// ── RTC-retained variables (survive deep sleep) ──
RTC_DATA_ATTR uint8_t      rtcBatteryLevel    = 0;
RTC_DATA_ATTR unsigned long rtcLastBatteryDrop = 0;
RTC_DATA_ATTR bool         rtcBatteryInit     = false;

// ── Globals ──
BleMouse bleMouse("MX Master 4", "Logitech", 100);

unsigned long lastBatteryDrop   = 0;
unsigned long lastConnectedTime = 0;
unsigned long lastFluctuation   = 0;

const unsigned long DISCONNECT_TIMEOUT   = 300000;     // 5 minutes
const uint64_t     DEEP_SLEEP_US        = 30 * 1000000ULL;   // 30 seconds
const unsigned long FLUCTUATION_INTERVAL = 7200000UL;  // 2 hours

// ── Dynamic drain interval based on battery level ──
unsigned long getDropInterval() {
  if (rtcBatteryLevel > 80) return 21600000UL;    // 6 h  (top plateau)
  if (rtcBatteryLevel > 30) return 14400000UL;    // 4 h  (normal range)
  if (rtcBatteryLevel > 10) return  7200000UL;    // 2 h  (low warning)
  return                      3600000UL;           // 1 h  (critical)
}

// ── Helper: maybe do an idle pause (10–100 s) with ~10% chance ──
void maybeIdlePause() {
  if (esp_random() % 100 < 10) {                    // ~10 % chance per move
    unsigned long pause = 10000 + (esp_random() % 90001);   // 10–100 s
    delay(pause);
    if (bleMouse.isConnected()) {
      lastConnectedTime = millis();
    }
  }
}

void setup() {
  // ── Battery persistence ──
  if (!rtcBatteryInit || rtcBatteryLevel < 15) {
    rtcBatteryLevel = 60 + (esp_random() % 41);   // 60 … 100
    rtcBatteryInit = true;
  }
  bleMouse.batteryLevel = rtcBatteryLevel;

  // ── Restore battery-drop timer ──
  lastBatteryDrop = millis();
  if (lastBatteryDrop < rtcLastBatteryDrop) {
    rtcLastBatteryDrop = lastBatteryDrop;
  } else {
    lastBatteryDrop = rtcLastBatteryDrop;
  }

  // ── Start BLE advertising ──
  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    unsigned long now = millis();

    // ── 1. Battery drain (non-linear curve) ──
    if (now - lastBatteryDrop >= getDropInterval()) {
      if (rtcBatteryLevel > 3) {
        rtcBatteryLevel--;
        bleMouse.setBatteryLevel(rtcBatteryLevel);
      }
      lastBatteryDrop   = now;
      rtcLastBatteryDrop = now;
    }

    // ── 2. Random fluctuations ±1 % every ~2 h ──
    if (now - lastFluctuation >= FLUCTUATION_INTERVAL) {
      int8_t delta = (int8_t)(esp_random() % 3) - 1;   // -1, 0, +1
      int16_t newLevel = (int16_t)rtcBatteryLevel + delta;
      if (newLevel > 100) newLevel = 100;
      if (newLevel < 3)   newLevel = 3;
      rtcBatteryLevel = (uint8_t)newLevel;
      bleMouse.setBatteryLevel(rtcBatteryLevel);
      lastFluctuation = now;
    }

    // ── Connected timestamp (for deep sleep timeout) ──
    lastConnectedTime = now;

    // ── Square pattern with random idle pauses ──
    bleMouse.move( 1, 0);
    delay(esp_random() % 20001);   // 0–20 s
    maybeIdlePause();

    bleMouse.move(-1, 0);
    delay(esp_random() % 20001);
    maybeIdlePause();

    bleMouse.move( 0, 1);
    delay(esp_random() % 20001);
    maybeIdlePause();

    bleMouse.move( 0, -1);
    delay(esp_random() % 20001);
    maybeIdlePause();

  } else {
    // ── Disconnected ─────────────────────────────────
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
