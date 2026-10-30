#include "jot/reader_review.h"

void jot_reader_review_reset(JotReaderReview *state) {
  if (!state) return;
  *state = (JotReaderReview){0};
}

JotReaderReviewAction jot_reader_review_select(JotReaderReview *state) {
  if (!state) return JOT_REVIEW_NONE;
  if (!state->open) {
    state->open = true;
    state->delete_mode = false;
    state->armed = false;
    return JOT_REVIEW_NONE;
  }
  if (!state->delete_mode) return JOT_REVIEW_EDIT;
  if (!state->armed) {
    state->armed = true;
    return JOT_REVIEW_NONE;
  }
  // Two distinct short presses in Delete mode are required. Never delete
  // on a long hold or on the first short Select.
  state->armed = false;
  return JOT_REVIEW_CONFIRM_DELETE;
}

void jot_reader_review_hold(JotReaderReview *state) {
  if (!state) return;
  state->open = true;
  state->delete_mode = true;
  state->armed = false;
}

bool jot_reader_review_cancel_armed(JotReaderReview *state) {
  if (!state || !state->armed) return false;
  state->armed = false;
  return true;
}

bool jot_reader_review_back(JotReaderReview *state) {
  if (!state) return false;
  if (jot_reader_review_cancel_armed(state)) return true;
  if (!state->open) return false;
  jot_reader_review_reset(state);
  return true;
}

int jot_reader_review_next_rank(int rank, int count, int direction) {
  if (count <= 0 || rank < 0 || rank >= count) return -1;
  if (direction < 0) return rank > 0 ? rank - 1 : rank;
  if (direction > 0) return rank + 1 < count ? rank + 1 : rank;
  return rank;
}
