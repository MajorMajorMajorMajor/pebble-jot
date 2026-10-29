#pragma once

// Pure layout/hit-testing helpers shared by drawing and touch handling.
// The cards occupy a clipped strip; header and footer are never scrollable.
enum {
  JOT_LIST_TOUCH_HEADER_H = 43,
  JOT_LIST_TOUCH_FOOTER_H = 29,
  JOT_LIST_TOUCH_ROWS = 3,
  JOT_LIST_CARD_X = 7,
  JOT_LIST_CARD_Y_PAD = 3
};

int jot_list_touch_row_height(int screen_height);
int jot_list_touch_max_scroll(int count, int screen_height);
int jot_list_touch_clamp_scroll(int pixels, int count, int screen_height);
int jot_list_touch_scroll_for_focus(int focus, int count, int screen_height);
// When physical navigation resumes after dragging, select the topmost note
// whose *drawn card* (not just logical row) is fully inside the viewport.
// Returns -1 for an empty/non-renderable list.
int jot_list_touch_first_full_card(int pixels, int count, int screen_height);

// Returns a newest-first rank only when x/y fall inside the actual drawn
// card (not the gaps, header, footer or unfilled rows). -1 otherwise.
int jot_list_touch_hit_card(int x, int y, int screen_width,
                            int screen_height, int count, int pixels);
