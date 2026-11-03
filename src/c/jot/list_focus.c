#include "jot/list_focus.h"

int jot_list_focus_initial(int rank, int count) {
  if (count <= 0) return JOT_LIST_FOCUS_NEW;
  if (rank < 0) return 0;
  return rank < count ? rank : count - 1;
}

int jot_list_focus_up(int focus) {
  return focus > JOT_LIST_FOCUS_NEW ? focus - 1 : JOT_LIST_FOCUS_NEW;
}

int jot_list_focus_down(int focus, int count) {
  return focus + 1 < count ? focus + 1 : focus;
}

bool jot_list_back_should_scroll(int scroll_px) {
  return scroll_px > 0;
}
