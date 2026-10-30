#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ui/kinetic_scroll.h"

static void flick_and_clamp(void) {
  JotKinetic m = {0};
  jot_kinetic_begin(&m, 1000);
  jot_kinetic_sample(&m, 40, 1040); // 1000px/sec finger-up flick
  assert(jot_kinetic_release(&m, 1050));
  int delta = 0, distance = 0, frames = 0;
  int prior = m.velocity;
  for (uint32_t t = 1070; m.active && t < 3000; t += 20) {
    assert(jot_kinetic_frame(&m, t, &delta));
    assert(delta >= 0);
    distance += delta;
    assert(m.velocity <= prior);
    prior = m.velocity;
    ++frames;
  }
  assert(frames > 12 && frames < 90);
  assert(distance > 90 && distance < 500);
  assert(!m.active);
  assert(!jot_kinetic_frame(&m, 3200, &delta));
  assert(delta == 0);
  jot_kinetic_begin(&m, 4000);
  jot_kinetic_sample(&m, -75, 4050);
  assert(jot_kinetic_release(&m, 4050));
  assert(jot_kinetic_frame(&m, 4070, &delta));
  assert(delta < 0);
  // A caller reaching a scroll boundary must stop the remaining momentum.
  jot_kinetic_cancel(&m);
  assert(!m.active);
}

static void stop_and_tap_safety(void) {
  JotKinetic m = {0};
  jot_kinetic_begin(&m, 100);
  jot_kinetic_sample(&m, 2, 200);
  assert(!jot_kinetic_release(&m, 215)); // slow/no flick
  jot_kinetic_begin(&m, 1000);
  jot_kinetic_sample(&m, 50, 1030);
  assert(!jot_kinetic_release(&m, 1200)); // paused before lift
  jot_kinetic_begin(&m, 2000);
  assert(!jot_kinetic_release(&m, 2100)); // stationary tap
  jot_kinetic_begin(&m, 3000);
  jot_kinetic_sample(&m, 30, 3040);
  assert(jot_kinetic_release(&m, 3040));
  jot_kinetic_cancel(&m); // next touchdown: stop instantly
  assert(!m.active && !m.tracking);
  assert(!jot_kinetic_frame(&m, 3060, &(int){0}));
}

static void sample_and_wrap(void) {
  JotKinetic m = {0};
  jot_kinetic_begin(&m, 1000);
  jot_kinetic_sample(&m, 20, 1002);
  jot_kinetic_sample(&m, 20, 1020);
  assert(m.velocity == JOT_KINETIC_MAX_SPEED);
  jot_kinetic_sample(&m, -30, 1040);
  assert(m.velocity < 1600); // reversal updates velocity before release
  jot_kinetic_cancel(&m);
  jot_kinetic_begin(&m, UINT32_MAX - 20u);
  jot_kinetic_sample(&m, 35, 20u);
  assert(jot_kinetic_release(&m, 25u));
  int step = 0;
  assert(jot_kinetic_frame(&m, 45u, &step));
  assert(step > 0);
  assert(!jot_kinetic_release(NULL, 100));
}

int main(void) {
  flick_and_clamp();
  stop_and_tap_safety();
  sample_and_wrap();
  puts("Integer-only kinetic scrolling speed, damping, stop and wrap passed");
  return 0;
}
