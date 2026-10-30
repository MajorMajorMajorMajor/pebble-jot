#include "layout.h"
#include "common.h"

static const LayoutKeyPosition english_abc_keys[] = {
  {KEY_ENGLISH_A, 0, 1, 1, 1},
  {KEY_ENGLISH_N, 0, 2, 1, 1},
  {KEY_ENGLISH_I, 0, 3, 1, 1},
  {KEY_ENGLISH_H, 1, 1, 1, 1},
  {KEY_ENGLISH_O, 1, 2, 1, 1},
  {KEY_ENGLISH_R, 1, 3, 1, 1},
  {KEY_ENGLISH_T, 2, 1, 1, 1},
  {KEY_ENGLISH_E, 2, 2, 1, 1},
  {KEY_ENGLISH_S, 2, 3, 1, 1},

  {KEY_ENGLISH_MENU,      0, 0, 1, 1},
  {KEY_ENGLISH_123,       1, 0, 1, 1},
  {KEY_ENGLISH_BACKSPACE, 2, 0, 1, 1},
  {KEY_ENGLISH_RETURN,    3, 0, 1, 1},

  {KEY_ENGLISH_SPACEBAR,  3, 1, 1, 3},
};

// 123 has its own 2-column ZERO and single-column Space; ABC unchanged.
static const LayoutKeyPosition english_123_keys[] = {
  {KEY_NUMBER_7, 0, 1, 1, 1},
  {KEY_NUMBER_8, 0, 2, 1, 1},
  {KEY_NUMBER_9, 0, 3, 1, 1},
  {KEY_NUMBER_4, 1, 1, 1, 1},
  {KEY_NUMBER_5, 1, 2, 1, 1},
  {KEY_NUMBER_6, 1, 3, 1, 1},
  {KEY_NUMBER_1, 2, 1, 1, 1},
  {KEY_NUMBER_2, 2, 2, 1, 1},
  {KEY_NUMBER_3, 2, 3, 1, 1},
  {KEY_ENGLISH_MENU,      0, 0, 1, 1},
  {KEY_NUMBER_ABC,        1, 0, 1, 1},
  {KEY_ENGLISH_BACKSPACE, 2, 0, 1, 1},
  {KEY_ENGLISH_RETURN,    3, 0, 1, 1},
  {KEY_NUMBER_ZERO,       3, 1, 1, 2},
  {KEY_NUMBER_SPACEBAR,   3, 3, 1, 1},
};

static const Layout s_layouts[LAYOUT_COUNT] = { 
  [LAYOUT_ENGLISH_ABC] = {
    .keys = english_abc_keys,
    .key_count = ARRAY_SIZE(english_abc_keys), // array size
    .rows = 4,
    .cols = 4
  },
  [LAYOUT_ENGLISH_123] = {
    .keys = english_123_keys,
    .key_count = ARRAY_SIZE(english_123_keys),
    .rows = 4,
    .cols = 4
  }
};

const Layout *get_layout(LayoutId layout_id) {
  return &s_layouts[layout_id];
}
