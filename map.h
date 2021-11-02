#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef map_h
#define map_h

typedef struct {
    size_t count, size;
    char **keys;
    void **values;
} map;

map * map_init();
map * map_init_size(size_t);
void map_deinit(map *);
// void map_insert(Hashable *, void *, map *);
// void * map_remove(Hashable *, map *);
// void * map_get(Hashable *, map *);
// int map_contains(Hashable *, map *);

void map_insert(char *, void *, map *);
void * map_get(char *, map *);
int map_contains(char *, map *);

#endif
#pragma once