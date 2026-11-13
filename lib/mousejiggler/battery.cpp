#include "battery.h"
#include "config.h"

#ifdef ESP_PLATFORM
#include <esp_system.h>
static uint32_t defaultRng() { return esp_random(); }
#else
#include <cstdlib>
static uint32_t defaultRng() { return static_cast<uint32_t>(rand()); }
#endif

BatterySimulator::BatterySimulator(RngFunc rng)
  : level_(0), lastDrop_(0), lastFluctuation_(0),
    lastTemp_(0.0f), firstDropInterval_(0),
    initialised_(false), rng_(rng ? rng : defaultRng),
    heatDropScheduled_(false), heatDropTime_(0) {}

uint8_t BatterySimulator::level() const { return level_; }
unsigned long BatterySimulator::lastDrop() const { return lastDrop_; }
unsigned long BatterySimulator::lastFluctuation() const { return lastFluctuation_; }
unsigned long BatterySimulator::firstDropInterval() const { return firstDropInterval_; }
void BatterySimulator::setLastDrop(unsigned long t) { lastDrop_ = t; }
void BatterySimulator::setLastFluctuation(unsigned long t) { lastFluctuation_ = t; }
void BatterySimulator::setFirstDropInterval(unsigned long t) { firstDropInterval_ = t; }
void BatterySimulator::setLevel(uint8_t lvl) { level_ = lvl; }

void BatterySimulator::init(uint8_t savedLevel, bool wasInitialised, unsigned long savedFirstDropInterval) {
  if (!wasInitialised || savedLevel < BATTERY_RECOVERY_MIN) {
    uint32_t range = BATTERY_INIT_MAX - BATTERY_INIT_MIN + 1;
    level_ = BATTERY_INIT_MIN + (rng_() % range);
  } else {
    level_ = savedLevel;
  }
  initialised_ = true;
  heatDropScheduled_ = false;
  heatDropTime_ = 0;

  // If starting at 100 %, use saved interval or generate a random 10-20 min one
  if (level_ == BATTERY_MAX) {
    if (savedFirstDropInterval > 0) {
      firstDropInterval_ = savedFirstDropInterval;
    } else {
      uint32_t range = DRAIN_FIRST_DROP_MAX - DRAIN_FIRST_DROP_MIN + 1;
      firstDropInterval_ = DRAIN_FIRST_DROP_MIN + (rng_() % range);
    }
  } else {
    firstDropInterval_ = 0;
  }
}

void BatterySimulator::update(unsigned long now, float tempCelsius) {
  // Use first-drop interval (10-20 min) when at 100 %, otherwise normal band
  unsigned long interval = (level_ == BATTERY_MAX && firstDropInterval_ > 0)
    ? firstDropInterval_
    : getDropInterval();

  // ── Normal drain (1 % at interval boundary) ──
  if (now - lastDrop_ >= interval) {
    if (level_ > BATTERY_MIN) {
      level_--;
    }
    lastDrop_ = now;
    lastTemp_ = tempCelsius;
    // After draining from 100 %, clear the special first-drop interval
    firstDropInterval_ = 0;
    // Reset any pending heat drop – we're in a new interval now
    heatDropScheduled_ = false;
  }

  // ── Heat‑triggered extra drain ──
  // If chip is hotter than last measurement and no heat drop is scheduled yet,
  // schedule one at a random offset within the current interval.
  if (lastTemp_ > 0.0f && tempCelsius > lastTemp_ && !heatDropScheduled_) {
    // Random offset between 0 and (interval - 1) ms from the last drop
    uint32_t offset = rng_() % interval;
    heatDropTime_ = lastDrop_ + offset;
    heatDropScheduled_ = true;
  }
  // Fire the scheduled heat drop only if chip is still hotter than baseline
  if (heatDropScheduled_ && now >= heatDropTime_ && level_ > BATTERY_MIN) {
    if (tempCelsius > lastTemp_) {
      level_--;
      // Update baseline so we don't schedule another heat drop until the
      // next normal drain (which also updates lastTemp_).
      lastTemp_ = tempCelsius;
    }
    heatDropScheduled_ = false;
  }

  // ── Fluctuation ──
  if (now - lastFluctuation_ >= interval) {
    int8_t delta = -static_cast<int8_t>(rng_() % 2);
    int16_t newLevel = static_cast<int16_t>(level_) + delta;
    if (newLevel > BATTERY_MAX) newLevel = BATTERY_MAX;
    if (newLevel < BATTERY_MIN) newLevel = BATTERY_MIN;
    level_ = static_cast<uint8_t>(newLevel);
    lastFluctuation_ = now;
  }
}

unsigned long BatterySimulator::getDropInterval() const {
  if (level_ > 80) return DRAIN_6H;
  if (level_ > 30) return DRAIN_4H;
  if (level_ > 10) return DRAIN_2H;
  return DRAIN_1H;
}
