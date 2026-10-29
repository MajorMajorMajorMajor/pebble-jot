#include "jot/list_focus.h"

int jot_list_focus_initial(int rank, int count) {
  if (count <= 0) return JOT_LIST_FOCUS_NEW;
  return rank >= 0 && rank < count ? rank : 0;
}

int jot_list_focus_up(int focus) {
  return focus > JOT_LIST_FOCUS_NEW ? focus - 1 : JOT_LIST_FOCUS_NEW;
}

int jot_list_focus_down(int focus, int count) {
  return focus + 1 < count ? focus + 1 : focus;
}

int jot_list_focus_back(int focus, int count) {
  if (focus > 0 || (focus == JOT_LIST_FOCUS_NEW && count > 0))
    return 0;
  return JOT_LIST_FOCUS_EXIT;
}
