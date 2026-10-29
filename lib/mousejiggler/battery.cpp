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
    initialised_(false), rng_(rng ? rng : defaultRng) {}

uint8_t BatterySimulator::level() const { return level_; }
unsigned long BatterySimulator::lastDrop() const { return lastDrop_; }
void BatterySimulator::setLastDrop(unsigned long t) { lastDrop_ = t; }
void BatterySimulator::setLastFluctuation(unsigned long t) { lastFluctuation_ = t; }
void BatterySimulator::setLevel(uint8_t lvl) { level_ = lvl; }

void BatterySimulator::init(uint8_t savedLevel, bool wasInitialised) {
  if (!wasInitialised || savedLevel < BATTERY_RECOVERY_MIN) {
    uint32_t range = BATTERY_INIT_MAX - BATTERY_INIT_MIN + 1;
    level_ = BATTERY_INIT_MIN + (rng_() % range);
    initialised_ = true;
  } else {
    level_ = savedLevel;
    initialised_ = true;
  }
}

void BatterySimulator::update(unsigned long now) {
  if (now - lastDrop_ >= getDropInterval()) {
    if (level_ > BATTERY_MIN) level_--;
    lastDrop_ = now;
  }
  if (now - lastFluctuation_ >= getDropInterval()) {
    int8_t delta = -static_cast<int8_t>(rng_() % 2);  // -1 or 0, never increase
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
