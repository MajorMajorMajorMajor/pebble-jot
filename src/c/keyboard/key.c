#include "keyboard/key.h"
#include "keyboard/key_label.h"

// Peripheral ABC legends are painted directly on the key face (no extra
// KeyLabel layers) using the same 8-direction layout as the numeric face.
static const Key s_keys[KEY_COUNT] = {  
  [KEY_ENGLISH_A] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "a" },      
    }
  },
  [KEY_ENGLISH_N] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "n" },
    }
  },
  [KEY_ENGLISH_I] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "i" },
    }
  },
  [KEY_ENGLISH_H] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "h" },
    }
  },
  [KEY_ENGLISH_O] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "o" },
    }
  },
  [KEY_ENGLISH_R] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "r" },
    }
  },  
  [KEY_ENGLISH_T] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "t" },
    }
  },  
  [KEY_ENGLISH_E] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "e" },
    }
  },  
  [KEY_ENGLISH_S] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "s" },      
    }
  },  
  [KEY_ENGLISH_SPACEBAR] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "SPACE" },
    }
  },
  [KEY_ENGLISH_MENU] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "SAVE" },
    }
  },
  [KEY_ENGLISH_123] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "123" },
    }
  },
  [KEY_ENGLISH_BACKSPACE] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "<==" },
    }
  },
  [KEY_ENGLISH_RETURN] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "RET" },
    }
  },
  // Numeric edge legends render in key_button.c (no additional Pebble layers).
  [KEY_NUMBER_7] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "7" } } },
  [KEY_NUMBER_8] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "8" } } },
  [KEY_NUMBER_9] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "9" } } },
  [KEY_NUMBER_4] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "4" } } },
  [KEY_NUMBER_5] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "5" } } },
  [KEY_NUMBER_6] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "6" } } },
  [KEY_NUMBER_1] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "1" } } },
  [KEY_NUMBER_2] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "2" } } },
  [KEY_NUMBER_3] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "3" } } },
  [KEY_NUMBER_ZERO] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "0" } } },
  [KEY_NUMBER_ABC] = { .labels = {
    { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "ABC" } } },
  // Blank numeric Space: the separate cell still receives touch feedback.
  [KEY_NUMBER_SPACEBAR] = { .labels = { {0} } },
};

const Key *get_key(KeyId key_id) {
  return &s_keys[key_id];
}
