#include "timing.h"
#include "config.h"

#ifdef ESP_PLATFORM
#include <esp_system.h>
static uint32_t defaultRng() { return esp_random(); }
#else
#include <cstdlib>
static uint32_t defaultRng() { return static_cast<uint32_t>(rand()); }
#endif

static const Move kSequence[MOVES_PER_SUPER_CYCLE] = {
  // Box 1
  { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
  // Diagonal 1
  { 1, 1}, {-1,-1},
  // Box 2
  { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
  // Diagonal 2
  { 1,-1}, {-1, 1},
  // Box 3
  { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
};

TimingGenerator::TimingGenerator(RngFunc rng)
  : moveIdx_(0), rng_(rng ? rng : defaultRng) {}

void TimingGenerator::reset() { moveIdx_ = 0; }

Move TimingGenerator::nextMove() {
  Move m = kSequence[moveIdx_];
  moveIdx_ = (moveIdx_ + 1) % MOVES_PER_SUPER_CYCLE;
  return m;
}

unsigned long TimingGenerator::moveDelay() {
  return rng_() % MOVE_DELAY_MAX;
}

bool TimingGenerator::maybeIdlePause() {
  return (rng_() % 100) < IDLE_PAUSE_PCT;
}

unsigned long TimingGenerator::idlePauseDuration() {
  uint32_t range = IDLE_PAUSE_MAX - IDLE_PAUSE_MIN + 1;
  return IDLE_PAUSE_MIN + (rng_() % range);
}
