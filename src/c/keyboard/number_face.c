#include "keyboard/number_face.h"

const char *jot_number_face_binding(int key, int gesture) {
  static const char *const numbers[9] = {
    "7", "8", "9", "4", "5", "6", "1", "2", "3"
  };
  static const char *const punctuation[9] = {
    "(", ")", "/", "-", ".", ":", ",", "?", "!"
  };
  if (key >= 0 && key < 9) {
    if (gesture == 0) return numbers[key];
    if (gesture == 5) return punctuation[key];
    return "";
  }
  if (key == 9) {
    if (gesture == 0) return "0";
    if (gesture == 5 || gesture == 18) return " ";
    return ""; // Other wide-key swipes are routed to cursor controls.
  }
  return NULL; // Save, Backspace, Return, ABC use existing bindings.
}
