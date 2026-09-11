#include "layout.h"

static const Layout *s_active_layout;

void keyboard_init(){
  s_active_layout = get_layout(LAYOUT_ENGLISH_ABC);
}

void keyboard_deinit(){
  // not using any resources yet.
  return;
}

const Layout* keyboard_get_active_layout(){
  return s_active_layout;
}