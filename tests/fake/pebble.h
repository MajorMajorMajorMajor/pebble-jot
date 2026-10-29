#pragma once
// Minimal Pebble SDK definitions used by src/c/jot/store.c.
// Deliberately distinguish S_SUCCESS from S_TRUE to catch deletion regressions.
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <limits.h>

typedef int32_t status_t;

enum {
  S_SUCCESS = 0,
  S_TRUE = 1,
  E_ERROR = -1,
  E_DOES_NOT_EXIST = -3
};

int persist_read_data(uint32_t key, void *buffer, size_t buffer_size);
int persist_write_data(uint32_t key, const void *data, size_t size);
status_t persist_delete(uint32_t key);
