#ifndef dynamic_array_h
#define dynamic_array_h
#include <stdlib.h>

typedef struct {
    size_t count, size;
    void **data;
} array;

array * create_array();
void append(void *, array *);
#endif
#pragma once