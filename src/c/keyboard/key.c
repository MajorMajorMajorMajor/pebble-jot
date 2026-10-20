#include "keyboard/key.h"
#include "keyboard/key_label.h"

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
      { .slot = KEY_LABEL_RIGHT,  .type = KEY_LABEL_TYPE_TEXT, .text = "k" },
    }
  },
  [KEY_ENGLISH_O] = {
    .labels = {
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "o" },
      { .slot = KEY_LABEL_RIGHT,  .type = KEY_LABEL_TYPE_TEXT, .text = "b" },
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
      { .slot = KEY_LABEL_RIGHT,  .type = KEY_LABEL_TYPE_TEXT, .text = "z" },
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
      { .slot = KEY_LABEL_CENTER, .type = KEY_LABEL_TYPE_TEXT, .text = "KB" },
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

};

const Key *get_key(KeyId key_id) {
  return &s_keys[key_id];
}