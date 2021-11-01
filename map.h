#include <stdlib.h>
#include <stdio.h>
#include "Hashable.h"
#pragma once



typedef struct {
    size_t count, size;
    Hashable **keys;
    void **values;
} map;

map * map_init();
map * map_init_size(size_t);
void map_deinit(map *);
void map_insert(Hashable *, void *, map *);
void * map_remove(Hashable *, map *);
void * map_get(Hashable *, map *);
int map_contains(Hashable *, map *);