#ifndef BATTERY_H
#define BATTERY_H

#include <cstdint>
#include "config.h"

class BatterySimulator {
public:
  using RngFunc = uint32_t (*)();

  BatterySimulator(RngFunc rng = nullptr);
  void   init(uint8_t savedLevel, bool wasInitialised, unsigned long savedFirstDropInterval = 0);
  void   update(unsigned long now, float tempCelsius = -1.0f);
  uint8_t      level() const;
  unsigned long lastDrop() const;
  unsigned long lastFluctuation() const;
  unsigned long firstDropInterval() const;
  void         setLastDrop(unsigned long t);
  void         setLastFluctuation(unsigned long t);
  void         setFirstDropInterval(unsigned long t);
  void         setLevel(uint8_t lvl);

private:
  uint8_t       level_;
  unsigned long lastDrop_;
  unsigned long lastFluctuation_;
  float         lastTemp_;
  unsigned long firstDropInterval_;
  bool          initialised_;
  RngFunc       rng_;
  unsigned long getDropInterval() const;
};

#endif
