#ifndef TIMING_H
#define TIMING_H

#include <cstdint>
#include "config.h"

struct Move {
  int8_t dx;
  int8_t dy;
};

class TimingGenerator {
public:
  using RngFunc = uint32_t (*)();

  TimingGenerator(RngFunc rng = nullptr);
  void  reset();
  Move  nextMove();
  unsigned long moveDelay();
  bool          maybeIdlePause();
  unsigned long idlePauseDuration();

private:
  unsigned int moveIdx_;
  RngFunc      rng_;
};

#endif
