#pragma once
#include <stdbool.h>

// The list header's "+" is a real, button-focusable row above the first note.
// Non-negative values are newest-first note ranks.
enum {
  JOT_LIST_FOCUS_NEW = -1,
  JOT_LIST_FOCUS_EXIT = -2
};

// On an empty list, focus "+" automatically; otherwise focus a valid note.
int jot_list_focus_initial(int rank, int count);
int jot_list_focus_up(int focus);
int jot_list_focus_down(int focus, int count);
// Back is determined by the VISIBLE scroll offset, not the focus rank:
// a partially/fully scrolled list returns to the top, then Back exits.
// This intentionally works the same with button focus or touch browsing.
bool jot_list_back_should_scroll(int scroll_px);
