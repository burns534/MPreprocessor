#include "string.h"
#define DEFAULT_STRING_SIZE 8
#define STRING_LOAD_FACTOR 0.6

static inline void resize(string *s) {
    s->data = realloc(s->data, s->size *= 2);
}

static inline void error(char *m) {
    perror(m);
    exit(EXIT_FAILURE);
}

string * string_init() {
    string *result = malloc(sizeof(string));
    result->data = malloc(DEFAULT_STRING_SIZE);
    result->size = DEFAULT_STRING_SIZE;
    result->length = 0;
    return result;
}

string * string_init_fromstring(string *s) {
    string *result = malloc(sizeof(string));
    result->data = malloc(s->size);
    result->length = s->length;
    result->size = s->size;
    strcpy(result->data, s->data);
    return result;
}

string * string_init_fromcstring(char *s) {
    string *result = malloc(sizeof(string));
    result->length = strlen(s);
    result->size = result->length * 2;
    result->data = malloc(result->size);
    strcpy(result->data, s);
    return result;
}

void string_append_string(string *str, string *s) {
    if (!str || !s) error("Error: null parameter");
    if (s->length + str->length > s->size * STRING_LOAD_FACTOR)
        resize(s);
    strcpy(s->data + s->length, str->data);
    s->length += str->length;
}

void string_append_cstrin(char *str, string *s) {
    if (!str || !s) error("Error: null parameter");
    size_t len = strlen(str);
    if (len + s->length > s->size * STRING_LOAD_FACTOR)
        resize(s);
    strcpy(s->data + len, str);
    s->length += len;
}

void string_append_char(char c, string *s) {
    if (!c || !s) error("Error: null parameter");
    if (s->length + 1 > s->size * STRING_LOAD_FACTOR)
        resize(s);
    s->data[s->length] = c;
    s->data[++(s->length)] = 0;
}

const char * string_c_str(string *s) {
    return s->data; // might need to make a copy here
}