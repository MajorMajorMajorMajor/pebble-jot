#include <stddef.h>
#include <stdint.h>
#include "keyboard/number_face.h"

// Canonical docs/gesture-map.csv in runtime recognizer order:
// TAP, N, NE, E, SE, S, SW, W, NW; return N/NE/E/SE/S/SW/W/NW; CW, CCW.
// Reserved macro and Compose actions remain deliberately inactive.
static const char *const s_bindings[9][19] = {
  { "7", "", "", "-", "", "", "$", "", "", "", "", "÷", "", "", "¥", "", "", "¹", "¹" }, // 7
  { "8", "^", "´", "!", "\\", "", "/", "+", "`", "ˇ", "’", "¡", "—", "", "–", "×", "‘", "²", "²" }, // 8
  { "9", "", "\n", "", "€", "=", "", "?", "", "", "\n", "", "£", "±", "", "¿", "", "", "" }, // 9
  { "4", "", "%", "", "_", "", "[", "(", "{", "", "‰", "", "¬", "", "]", ")", "}", "¼", "¼" }, // 4
  { "5", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "a", "a" }, // 5
  { "6", "", "}", ")", "]", "", "@", "", "|", "", "{", "(", "[", "", "ª", "", "¦", "ⁿ", "ⁿ" }, // 6
  { "1", "¨", "", "*", "", "", "\b", "<", "~", "˝", "", "†", "", "", "\b", "‹", "˜", "∫", "∫" }, // 1
  { "2", "", "'", "", ":", ".", ",", "", "\"", "", "”", "", "„", "…", "‚", "", "“", "Π", "Π" }, // 2
  { "3", "&", "°", ">", " ", "", ";", "#", "", "§", "0", "3", " ", "", ";", "£", "", "Σ", "Σ" }, // 3
};

const char *jot_number_face_binding(int key, int gesture) {
  if (key >= 0 && key < 9 && gesture >= 0 && gesture < 19)
    return s_bindings[key][gesture];
  if (key == 9 && gesture >= 0 && gesture < 19) {
    // Wide zero retains spacebar navigation through the existing router.
    if (gesture == 0 || gesture == 17) return "0";
    if (gesture == 4 || gesture == 5 || gesture == 6 ||
        gesture == 12 || gesture == 13 || gesture == 14 ||
        gesture == 18) return " ";
    return "";
  }
  return NULL;
}


// The 8 straight swipes: NW, N, NE, E, SE, S, SW, W.
// Symbols that aren't safely printable on Pebble are not advertised
// as text; supported action hints are drawn as native little pictograms.
static const uint8_t s_straight_gestures[8] = {8,1,2,3,4,5,6,7};

JotNumberPictogram jot_number_face_pictogram(int key, int direction) {
  if (direction < 0 || direction >= 8) return JOT_NUM_PICTO_NONE;
  if (key == 2) { // 9: up arrow, Return
    if (direction == 1) return JOT_NUM_PICTO_UP;
    if (direction == 2) return JOT_NUM_PICTO_RETURN;
  } else if (key == 6) { // 1: only Backspace works.
    if (direction == 6) return JOT_NUM_PICTO_BACKSPACE;
  } else if (key == 8) { // 3: Space, cursor down
    if (direction == 4) return JOT_NUM_PICTO_SPACE;
    if (direction == 5) return JOT_NUM_PICTO_DOWN;
  }
  return JOT_NUM_PICTO_NONE;
}

const char *jot_number_face_legend(int key, int direction) {
  if (key < 0 || key >= 9 || direction < 0 || direction >= 8)
    return NULL;
  if (jot_number_face_pictogram(key, direction) != JOT_NUM_PICTO_NONE)
    return NULL;
  // Don't paint multi-byte Unicode punctuation with Gothic 14: many
  // valid storage glyphs have no Pebble font glyph and display as boxes.
  // Bindings still insert the exact canonical UTF-8 symbols; their
  // legends remain absent until we can supply verified pixel-art.
  const char *text = s_bindings[key][s_straight_gestures[direction]];
  return text[0] >= ' ' && text[0] <= '~' && text[1] == '\0' ?
         text : NULL;
}

int jot_number_face_bottom_touch(int col) {
  if (col == 1 || col == 2) return 9;
  return col == 3 ? 14 : -1;
}
