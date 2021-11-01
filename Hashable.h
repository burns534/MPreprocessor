#ifndef hashable_h
#define hashable_h

typedef struct Hashable {
    long hash;
    int (*equals) (struct Hashable *);
} Hashable;

#endif