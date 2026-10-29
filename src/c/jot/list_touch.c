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

int jot_list_touch_first_full_card(int pixels, int count, int screen_height) {
  const int row = jot_list_touch_row_height(screen_height);
  const int view = screen_height - JOT_LIST_TOUCH_HEADER_H -
                   JOT_LIST_TOUCH_FOOTER_H;
  if (count <= 0 || row <= 2 * JOT_LIST_CARD_Y_PAD ||
      view < row - 2 * JOT_LIST_CARD_Y_PAD) return -1;
  const int scroll = jot_list_touch_clamp_scroll(
      pixels, count, screen_height);

  // The *card* starts three pixels into its row. The first row can still be
  // fully visible during the first three pixels of scrolling, even though
  // its background/paper strip is cropped. Start at the earliest card
  // whose own top edge is at or below the clipped viewport's upper edge.
  const int beyond_pad = scroll - JOT_LIST_CARD_Y_PAD;
  const int first = beyond_pad > 0 ? (beyond_pad + row - 1) / row : 0;
  if (first >= count) return -1;

  const int card_top = first * row + JOT_LIST_CARD_Y_PAD - scroll;
  const int card_bottom = (first + 1) * row - JOT_LIST_CARD_Y_PAD - scroll;
  if (card_top < 0 || card_bottom > view) return -1;
  return first;
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
