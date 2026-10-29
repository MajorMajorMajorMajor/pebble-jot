#pragma once

void touch_init(void);
void touch_deinit(void);

// Temporary trace collection controls, invoked by the watch's physical buttons.
void touch_recorder_select(void);
void touch_recorder_back(void);
void touch_recorder_next_type(void);
void touch_recorder_next_direction(void);

// Long-select from Jot: retain the previous diagnostic harness.
void touch_enter_harness(void);
