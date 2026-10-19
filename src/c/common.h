#pragma once

#include <pebble.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define PEBBLE_ASSERT(expr) \
  do { \
    if (!(expr)) { \
      APP_LOG(APP_LOG_LEVEL_ERROR, \
              "Assertion failed: %s (%s:%d)", \
              #expr, __FILE__, __LINE__); \
      __builtin_trap(); \
    } \
  } while (0)
  