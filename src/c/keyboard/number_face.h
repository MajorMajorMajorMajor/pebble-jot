#pragma once

// A pure, SDK-independent mapping from physical touch index (0..13) to
// the numeric face. Only number cells (0..8) and the wide zero (9)
// are handled here. Controls and cursor commands retain their own routing.
// gesture: 0=Tap; 5=Down; 18=CCW, consistent with the alphabet face.
// Unknown gestures deliberately enter nothing, never hidden letters.
const char *jot_number_face_binding(int touch_index, int gesture_index);
