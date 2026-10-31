#pragma once
#include <stddef.h>

// Measure the natural, unwrapped width of text[first:last] in pixels.
// first and last must be UTF-8 codepoint boundaries.
typedef int (*JotCaretMeasureWidth)(const char *text, size_t first,
                                    size_t last, void *context);

typedef struct {
  int row;             // Zero-based visual row, counting LF and soft wraps.
  int x;               // Position in pixels from the left edge of that row.
  size_t line_start;   // Byte offset where the displayed row begins.
} JotCaretLayout;

// Calculate caret position using whole-word wrapping (including a possible
// break BEFORE each literal hyphen, as observed on Emery). Unlike measuring
// a backwards suffix, this lays out every complete word segment from the
// beginning, including the rest of the word to the right of a mid-word
// cursor. The callback uses the same font as the actual watch TextLayer.
// No note data is modified. The result excludes horizontal padding.
JotCaretLayout jot_caret_layout(const char *text, size_t cursor,
                                int width, JotCaretMeasureWidth measure,
                                void *context);
