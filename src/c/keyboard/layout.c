#include "layout.h"

static Layout english_abc_layout = { 
    (LayoutKeyPosition[]) {
      {KEY_ENGLISH_O, 0, 0, 1, 1},
      {KEY_ENGLISH_N, 0, 1, 1, 1}
    },  
    .key_count = 2, // array size
    .rows = 1, // max of (row + row_span)
    .cols = 2  // max of (col + col_span)
};