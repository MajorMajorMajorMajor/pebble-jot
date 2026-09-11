#include "layout.h"
#include "common.h"

static const LayoutKeyPosition english_abc_keys[] = {
  {KEY_ENGLISH_O, 0, 0, 1, 1},
  {KEY_ENGLISH_N, 0, 1, 1, 1}
};

static const Layout s_layouts[LAYOUT_COUNT] = { 
  [LAYOUT_ENGLISH_ABC] = {
    .keys = english_abc_keys,
    .key_count = ARRAY_SIZE(english_abc_keys), // array size
    .rows = 1, // max of (row + row_span)
    .cols = 2  // max of (col + col_span)}
  }
};

const Layout *get_layout(LayoutId layout_id) {
  return &s_layouts[layout_id];
}