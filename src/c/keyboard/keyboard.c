#include "layout.h"

static LayoutId s_active_layout_id;

void keyboard_init(){
  s_active_layout_id = LAYOUT_ENGLISH_ABC;
}

void keyboard_deinit(){
  // not using any resources yet.
  return;
}

const Layout *keyboard_get_active_layout(){
  return get_layout(s_active_layout_id);
}