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

static const Layout s_layouts[LAYOUT_COUNT] = { 
  [LAYOUT_ENGLISH_ABC] = {
    .keys = english_abc_keys,
    .key_count = ARRAY_SIZE(english_abc_keys), // array size
    .rows = 4,
    .cols = 4
  }
};

const Layout *get_layout(LayoutId layout_id) {
  return &s_layouts[layout_id];
}