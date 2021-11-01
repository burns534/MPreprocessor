#include <stdio.h>
#include <stdlib.h>
#include "Hashable.h"
#include <string.h>
#pragma once

typedef struct {
    Hashable;
    size_t length, size;
    char *data;
} string;

string * string_init();
string * string_init_fromstring(string *);
string * string_init_fromcstring(char *);
void string_append_string(string *, string *);
void string_append_cstrin(char *, string *);
void string_append_char(char, string *);
const char * string_c_str(string *);
