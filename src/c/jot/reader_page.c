#include "jot/reader_page.h"

int jot_reader_page_view_height(int screen_height) {
  return screen_height > JOT_READER_FOOTER_H ?
         screen_height - JOT_READER_FOOTER_H : 0;
}

int jot_reader_page_text_top(int scroll_y) {
  return JOT_READER_HEADER_H + JOT_READER_META_H +
         JOT_READER_TEXT_TOP_INSET - scroll_y;
}

int jot_reader_page_scroll_limit(int text_height, int view_height) {
  if (text_height < 0) text_height = 0;
  const int document_height = JOT_READER_HEADER_H + JOT_READER_META_H +
                              text_height + JOT_READER_BOTTOM_PAD;
  return document_height > view_height ?
         document_height - view_height : 0;
}

int jot_reader_page_first_rule(int scroll_y) {
  // Only rule lines belonging to actual paper (document y >= 93).
  // Advance in whole line-spacing increments, keeping the ruled-paper
  // phase fixed to its words even when dragging a pixel at a time.
  int y = JOT_READER_RULE_FIRST_Y - (scroll_y > 0 ? scroll_y : 0);
  if (y < 0)
    y += (-y + JOT_READER_RULE_SPACING - 1) /
         JOT_READER_RULE_SPACING * JOT_READER_RULE_SPACING;
  return y;
}

int jot_reader_page_visible_header(int scroll_y) {
  if (scroll_y <= 0) return JOT_READER_HEADER_H;
  if (scroll_y >= JOT_READER_HEADER_H) return 0;
  return JOT_READER_HEADER_H - scroll_y;
}

bool jot_reader_page_back_to_top(int scroll_y) {
  return scroll_y > 0;
}
