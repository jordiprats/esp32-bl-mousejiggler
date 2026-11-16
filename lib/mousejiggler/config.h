#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

// Battery
constexpr uint8_t  BATTERY_MIN          = 3;
constexpr uint8_t  BATTERY_MAX          = 100;
constexpr uint8_t  BATTERY_INIT_MIN     = 60;
constexpr uint8_t  BATTERY_INIT_MAX     = 100;
constexpr uint8_t  BATTERY_RECOVERY_MIN = 15;    // if saved < this, re-randomise

// Drain intervals (ms) per level band
constexpr unsigned long DRAIN_6H            = 21600000UL;   // >80 %
constexpr unsigned long DRAIN_4H            = 14400000UL;   // >30 %
constexpr unsigned long DRAIN_2H            =  7200000UL;   // >10 %
constexpr unsigned long DRAIN_1H            =  3600000UL;   // ≤10 %
constexpr unsigned long DRAIN_FIRST_DROP_MIN =   600000UL;   // 10 min (100→99 %)
constexpr unsigned long DRAIN_FIRST_DROP_MAX =  1200000UL;   // 20 min (100→99 %)

// Timing / movement
constexpr unsigned long MOVE_DELAY_MAX    = 20000UL;   // 0–20 s jitter
constexpr unsigned long IDLE_PAUSE_MIN    = 10000UL;   // 10 s
constexpr unsigned long IDLE_PAUSE_MAX    = 100000UL;  // 100 s
constexpr unsigned int  IDLE_PAUSE_PCT    = 10;        // ~10 % per move

// Reconnect backoff
constexpr unsigned long INIT_RECONNECT_DELAY_MS = 5000UL;      // 5 s
constexpr unsigned long MAX_RECONNECT_DELAY_MS   = 180000UL;    // 3 min

// Fake SSID beacon (WiFi AP mode) – camouflages the device as a WiFi access point.
// The device cycles through up to 3 SSIDs so a scanner sees different networks.
#ifndef FAKE_SSID_ENABLED
#define FAKE_SSID_ENABLED true
#endif
#ifndef FAKE_SSID
#define FAKE_SSID "iPhone"
#endif

// Power / CPU
constexpr uint8_t CPU_FREQ_MHZ = 80;   // MHz - 80 is the lowest, at 40 BL radio dont work

// Movement pattern
constexpr unsigned int MOVES_PER_SUPER_CYCLE = 16;

#endif
