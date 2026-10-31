// Key-label position behavior; pure C so no Pebble SDK is required.
// Intentionally test the baseline and invariants without freezing the
// numeric optical offsets the user is calibrating on real hardware.
#include <assert.h>
#include <stdio.h>
#include "ui/legend_geometry.h"

static JotLabelMetrics metrics(int w, int h, int text_w, int font_h) {
  return (JotLabelMetrics){
    .box_x = 1, .box_y = 1, .box_w = w - 2, .box_h = h - 2,
    .text_w = text_w, .font_h = font_h, .padding = 3,
    .bounds_w = w, .bounds_h = h
  };
}

static void verify_base_frames(void) {
  const int sizes[][2] = {{46, 45}, {55, 58}, {80, 62}, {31, 37}};
  for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    JotLabelMetrics m = metrics(sizes[i][0], sizes[i][1], 17, 32);
    JotLabelFrame center = jot_key_text_frame(
        JOT_TEXT_CENTER, JOT_LEGEND_ABC, -1, m);
    assert(center.x == m.box_x + m.box_w / 2 - m.text_w / 2);
    assert(center.y == m.box_y + m.box_h / 2 - m.font_h / 2 - 2);
    assert(center.w == m.text_w && center.h == m.font_h);

    JotLabelFrame right = jot_key_text_frame(
        JOT_TEXT_RIGHT, JOT_LEGEND_NUMBER, 4, m);
    assert(right.x == m.box_x + m.box_w - m.text_w - m.padding);
    assert(right.y == m.box_y + m.box_h / 2 - m.font_h / 2);
    assert(right.w == m.text_w && right.h == m.font_h);

    JotLabelFrame bottom = jot_key_text_frame(
        JOT_TEXT_BOTTOM, JOT_LEGEND_ABC, 4, m);
    assert(bottom.x == m.box_x + m.box_w / 2 - m.text_w / 2);
    assert(bottom.y == (int)(m.box_y + m.box_h -
                             m.font_h * 0.75 - m.padding));
    assert(bottom.w == m.text_w && bottom.h == m.font_h);

    // Zero's previous special-case draw rectangle is still the baseline.
    JotLabelFrame zero = jot_key_text_frame(
        JOT_TEXT_ZERO, JOT_LEGEND_ABC, -1, m);
    assert(zero.x == 0 && zero.y == m.bounds_h / 2 - 22);
    assert(zero.w == m.bounds_w && zero.h == 36);

    // Future optical tuning must not change dimensions, and only centre
    // positions are permitted to be affected by centre offset tables.
    for (int face = JOT_LEGEND_ABC; face <= JOT_LEGEND_NUMBER; ++face) {
      for (int key = 0; key < 9; ++key) {
        JotLabelFrame f = jot_key_text_frame(
            JOT_TEXT_CENTER, (JotLegendFace)face, key, m);
        assert(f.w == center.w && f.h == center.h);
        JotLabelFrame edge = jot_key_text_frame(
            JOT_TEXT_RIGHT, (JotLegendFace)face, key, m);
        assert(edge.x == right.x && edge.y == right.y);
      }
    }
  }
}

int main(void) {
  verify_base_frames();
  puts("Text label geometry: center, zero and legacy edge slots passed");
  return 0;
}
