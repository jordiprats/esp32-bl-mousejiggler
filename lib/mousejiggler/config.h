#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

// LED GPIO – set to -1 if no LED, or override in config.local.h
#ifndef LED_GPIO
#if defined(LED_BUILTIN)
#define LED_GPIO LED_BUILTIN
#else
#define LED_GPIO 2
#endif
#endif

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

// Deep sleep
constexpr unsigned long DISCONNECT_TIMEOUT = 300000UL;       // 5 min
constexpr uint64_t     DEEP_SLEEP_US      = 30 * 1000000ULL;   // 30 s

// LED flash – GPIO for a 3× 500 ms blink before deep sleep
// Set to -1 to disable. Override via LED_GPIO in config.local.h.
constexpr int kLedGpio = LED_GPIO;

// Reconnect backoff
constexpr unsigned long INIT_RECONNECT_DELAY_MS = 5000UL;      // 5 s
constexpr unsigned long MAX_RECONNECT_DELAY_MS   = 180000UL;    // 3 min
constexpr unsigned long COLD_BOOT_DELAY_MS        = 600000UL;   // 10 min

// Deep sleep escalation
constexpr uint64_t MAX_DEEP_SLEEP_US = 180000000ULL;           // 3 min

// Power / CPU
constexpr uint8_t CPU_FREQ_MHZ = 80;   // MHz - 80 is the lowest, at 40 BL radio dont work

// Movement pattern
constexpr unsigned int MOVES_PER_SUPER_CYCLE = 16;

#endif
