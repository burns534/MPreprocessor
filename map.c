#include "map.h"
#define DEFAULT_MAP_SIZE 8
#define MAP_LOAD_FACTOR 0.6

static Hashable *DUMMY = (Hashable *) 1; // I doubt malloc ever returns 1 lol
// utility

static void error(char *m) {
    perror(m);
    exit(EXIT_FAILURE);
}

static void resize(map *m) {
    size_t old_size = m->size;
    m->size *= 2;
    Hashable **new_keys = malloc(sizeof(Hashable *) * m->size);
    void ** new_values = malloc(sizeof(void *) * m->size);

    for (int i = 0; i < old_size; i++) {
        if (m->keys[i] && m->keys[i] != DUMMY) {
            // insert value
            long index = m->keys[i]->hash % m->size;
            while(m->keys[index]) {
                index++;
                index %= m->size;
            }

            new_keys[index] = m->keys[i];
            new_values[index] = m->values[i];
        }
    }

    free(m->keys);
    free(m->values);

    m->keys = new_keys;
    m->values = new_values;
}

// main methods
map * map_init() {
    map *result = malloc(sizeof(map));
    result->count = 0;
    result->size = DEFAULT_MAP_SIZE;
    result->keys = malloc(sizeof(Hashable *) * DEFAULT_MAP_SIZE);
    result->values = malloc(sizeof(void *) * DEFAULT_MAP_SIZE);
    return result;
}

map * map_init_size(size_t size) {
    map *result = malloc(sizeof(map));
    result->count = 0;
    result->size = size;
    result->keys = malloc(sizeof(Hashable *) * size);
    result->values = malloc(sizeof(void *) * size);
    return result;
}

void map_deinit(map *m) {
    free(m->keys);
    free(m->values);
    free(m);
}

void map_insert(Hashable *key, void *value, map *m) {
    if (!key || !value || !m) error("Error: null parameter");

    if (m->count > m->size * MAP_LOAD_FACTOR)
        resize(m);
    
    size_t index = key->hash % m->size;
    while(m->keys[index] && m->keys[index] != DUMMY) 
        index = (index + 1) % m->size;

    m->keys[index] = key;
    m->values[index] = value;
}

void * map_remove(Hashable *key, map *m) {
    if (!key || !m) error("Error: null parameter");
    
    size_t index = key->hash % m->size;

    while(m->keys[index]) index = (index + 1) % m->size;
    
    void *result = m->values[index];
    m->keys[index] = DUMMY;
    m->values[index] = NULL;
    return result;
}

void * map_get(Hashable *key, map *m) {
    if (!key || !m) error("Error: null parameter");

    size_t index = key->hash % m->size;

    while(m->keys[index]) {
        if (m->keys[index]->equals(key)) return m->values[index];
        index = (index + 1) % m->size;
    }

    return NULL;
}

int map_contains(Hashable *key, map *m) {
    if (!key || !m) error("Error: null parameter");

    size_t index = key->hash % m->size;

    while(m->keys[index]) {
        if (m->keys[index]->equals(key)) return 1;
        index = (index + 1) % m->size;
    }

    return 0;
}