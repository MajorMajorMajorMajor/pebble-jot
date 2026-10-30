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
  { "1", "¨", "", "*", "\t", "", "\b", "<", "~", "˝", "", "†", "\t", "", "\b", "‹", "˜", "∫", "∫" }, // 1
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

// The 8 straight-swipe legends in NW, N, NE, E, SE, S, SW, W order.
const char *jot_number_face_legend(int key, int direction) {
  static const uint8_t g[8] = {8,1,2,3,4,5,6,7};
  if (key < 0 || key >= 9 || direction < 0 || direction >= 8)
    return NULL;
  if (key == 0 && direction == 0) return "C"; // Compose reserved
  if (key == 2 && direction == 1) return "↑";
  if (key == 2 && direction == 2) return "↵";
  if (key == 6 && direction == 4) return "⇥";
  if (key == 6 && direction == 6) return "⌫";
  if (key == 8 && direction == 4) return "—";
  if (key == 8 && direction == 5) return "↓";
  const char *text = s_bindings[key][g[direction]];
  return *text ? text : NULL;
}
