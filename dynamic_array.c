#include "dynamic_array.h"
#define DEFAULT_ARRAY_SIZE 8
#define ARRAY_LOAD_FACTOR 0.6

static inline void resize(array *a) {
    a->data = realloc(a->data, a->size *= 2);
}

void init_array(array *a) {
    a->count = 0;
    a->data = malloc(DEFAULT_ARRAY_SIZE);
    a->size = DEFAULT_ARRAY_SIZE;
}

array * create_array() {
    array *result = malloc(sizeof(array));
    result->count = 0;
    result->data = malloc(DEFAULT_ARRAY_SIZE);
    result->size = DEFAULT_ARRAY_SIZE;
    return result;
}

