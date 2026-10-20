#include "layout.h"

static LayoutId s_active_layout_id;

void keyboard_init(void) {
  s_active_layout_id = LAYOUT_ENGLISH_ABC;
}

void keyboard_deinit(void) {
  // not using any resources yet.
  return;
}

LayoutId keyboard_get_active_layout_id(void) {
  return s_active_layout_id;
}