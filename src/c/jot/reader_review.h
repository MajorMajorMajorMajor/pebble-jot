#pragma once
#include <stdbool.h>

// A reader-only mode switch. No note mutation occurs in this module.
// The caller must explicitly perform storage deletion on CONFIRM_DELETE.
typedef struct {
  bool open;
  bool delete_mode;
  bool armed;
} JotReaderReview;

typedef enum {
  JOT_REVIEW_NONE = 0,
  JOT_REVIEW_EDIT,
  JOT_REVIEW_CONFIRM_DELETE
} JotReaderReviewAction;

void jot_reader_review_reset(JotReaderReview *state);
JotReaderReviewAction jot_reader_review_select(JotReaderReview *state);
void jot_reader_review_hold(JotReaderReview *state);
// True if Back was consumed by cancelling confirmation or closing the bar.
bool jot_reader_review_back(JotReaderReview *state);
// True if a pending confirmation was cancelled (without moving notes).
bool jot_reader_review_cancel_armed(JotReaderReview *state);
// Return the next valid rank, or the current rank at an end boundary.
// direction = -1 for newer/Up, +1 for older/Down.
int jot_reader_review_next_rank(int rank, int count, int direction);

 
// The open native bar occupies the right ACTION_BAR_WIDTH pixels.
// Clamp pathological widths defensively. Peek and closed mode use full width.
int jot_reader_review_paper_width(int screen_width, bool bar_open,
                                  int bar_width);
