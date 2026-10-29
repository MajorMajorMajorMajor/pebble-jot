#pragma once

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
// Back from "+" returns to the top note, if one exists; Back at the top
// (or from "+" on an empty list) exits to the watch system.
int jot_list_focus_back(int focus, int count);
