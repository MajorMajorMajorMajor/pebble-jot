#pragma once
#include <stdbool.h>
#include "jot/reader_touch.h"

// A Reader is one continuous paper document. Only the 30px footer remains
// fixed; the 42px header, 33px timestamp band, ruled lines and note all
// share the same scroll offset. Helpers are SDK-independent and allocate 0B.
enum {
  JOT_READER_RULE_FIRST_Y = JOT_READER_HEADER_H + JOT_READER_META_H + 18,
  JOT_READER_RULE_SPACING = 30,
  JOT_READER_BOTTOM_PAD = 4
};
int jot_reader_page_view_height(int screen_height);
int jot_reader_page_text_top(int scroll_y);
int jot_reader_page_scroll_limit(int text_height, int view_height);
int jot_reader_page_first_rule(int scroll_y);
int jot_reader_page_visible_header(int scroll_y);
bool jot_reader_page_back_to_top(int scroll_y);
