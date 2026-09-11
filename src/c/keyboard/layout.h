#pragma once

#include <stddef.h>
#include "key.h"

typedef struct {
    KeyId key_id;
    unsigned int row;
    unsigned int col;
    unsigned int row_span;
    unsigned int col_span;
} LayoutKeyPosition;

typedef struct {
    const LayoutKeyPosition *keys; // 
    size_t key_count;
    unsigned int rows;
    unsigned int cols;
} Layout;

typedef enum {
    LAYOUT_ENGLISH_ABC,
    LAYOUT_ENGLISH_123,
    LAYOUT_COUNT
} LayoutId;


const Layout* get_layout(LayoutId layout_id);