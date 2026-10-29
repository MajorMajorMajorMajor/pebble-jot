#include "jot/list_touch.h"

static int prv_max(int x, int y) { return x > y ? x : y; }
static int prv_min(int x, int y) { return x < y ? x : y; }

int jot_list_touch_row_height(int screen_height) {
  const int view = screen_height - JOT_LIST_TOUCH_HEADER_H -
                   JOT_LIST_TOUCH_FOOTER_H;
  return view > JOT_LIST_TOUCH_ROWS ? view / JOT_LIST_TOUCH_ROWS : 0;
}

int jot_list_touch_max_scroll(int count, int screen_height) {
  if (count <= JOT_LIST_TOUCH_ROWS) return 0;
  const int row = jot_list_touch_row_height(screen_height);
  const int view = screen_height - JOT_LIST_TOUCH_HEADER_H -
                   JOT_LIST_TOUCH_FOOTER_H;
  return prv_max(0, count * row - view);
}

int jot_list_touch_clamp_scroll(int pixels, int count, int screen_height) {
  return prv_min(prv_max(pixels, 0),
                 jot_list_touch_max_scroll(count, screen_height));
}

int jot_list_touch_scroll_for_focus(int focus, int count, int screen_height) {
  if (focus < 0 || count <= JOT_LIST_TOUCH_ROWS) return 0;
  int first = prv_max(focus - 1, 0);
  first = prv_min(first, prv_max(count - JOT_LIST_TOUCH_ROWS, 0));
  return jot_list_touch_clamp_scroll(
      first * jot_list_touch_row_height(screen_height), count, screen_height);
}

int jot_list_touch_focus_at_scroll(int pixels, int count, int screen_height) {
  if (count <= 0) return -1;
  const int row = jot_list_touch_row_height(screen_height);
  if (row <= 0) return 0;
  // During finger scrolling, highlight a note near the center of the
  // viewport. The existing physical buttons then continue from it.
  const int view = screen_height - JOT_LIST_TOUCH_HEADER_H -
                   JOT_LIST_TOUCH_FOOTER_H;
  int focus = (jot_list_touch_clamp_scroll(pixels, count, screen_height)
               + view / 2) / row;
  return prv_min(prv_max(focus, 0), count - 1);
}

int jot_list_touch_hit_card(int x, int y, int screen_width,
                            int screen_height, int count, int pixels) {
  if (count <= 0 || x < JOT_LIST_CARD_X ||
      x >= screen_width - JOT_LIST_CARD_X ||
      y < JOT_LIST_TOUCH_HEADER_H ||
      y >= screen_height - JOT_LIST_TOUCH_FOOTER_H) return -1;
  const int row = jot_list_touch_row_height(screen_height);
  if (row <= 2 * JOT_LIST_CARD_Y_PAD) return -1;
  const int content_y = y - JOT_LIST_TOUCH_HEADER_H +
      jot_list_touch_clamp_scroll(pixels, count, screen_height);
  if (content_y < 0) return -1;
  const int rank = content_y / row;
  const int inside = content_y % row;
  if (rank < 0 || rank >= count ||
      inside < JOT_LIST_CARD_Y_PAD ||
      inside >= row - JOT_LIST_CARD_Y_PAD) return -1;
  return rank;
}
