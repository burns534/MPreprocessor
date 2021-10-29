#include <stdio.h>
#include <string.h> 
typedef struct {
	int *data;
	int count;
	int size;
} IntArray;
static const int _IntArray_DEFAULT_SIZE = 8;
static const double _IntArray_LOAD_FACTOR = 0.6;
IntArray * _IntArray_init() {
IntArray *self = malloc(sizeof(IntArray));
self->data = malloc(sizeof(int)* _IntArray_DEFAULT_SIZE);
self->size = _IntArray_DEFAULT_SIZE;
self->count = 0;
return self;
}
IntArray * _IntArray_init_fromArray_length(int *fromArray, int length) {
IntArray *self = malloc(sizeof(IntArray));
self->size = length * 2;
self->data = malloc(sizeof(int)* self->size);
self->count = length;
return self;
}
void _IntArray_deinit(IntArray *self) {
free(self->data);
free(self);
}
int  _IntArray_getValue_atIndex(int atIndex, IntArray *self) {
if(atIndex >= self->count)return - 1;
return self->data[atIndex];
}
void _IntArray_resize(IntArray *self) {
self->size *= 2;
self->data = realloc(sizeof(int)* self->size);
}
void _IntArray_append_value(int value, IntArray *self) {
if(self->count > _IntArray_LOAD_FACTOR * self->size){ _IntArray_resize(self);
}
self->data[self->count ++]= value;
}
int  _IntArray_pop(IntArray *self) {
return self->data[-- self->count];
}
int  _IntArray_remove_atIndex(int atIndex, IntArray *self) {
	const int result = self->data[atIndex];
for(int i = atIndex;
i < self->count - 1;
i ++)self->data[i]= self->data[i + 1];
return result;
}

typedef struct {
	char *data;
	int length;
	int size;
} String;
static const double _String_LOAD_FACTOR = 0.6;
static const int _String_DEFAULT_SIZE = 64;
String * _String_init() {
String *self = malloc(sizeof(String));
self->data = malloc(_String_DEFAULT_SIZE);
self->size = _String_DEFAULT_SIZE;
self->length = 0;
return self;
}
String * _String_init_fromString(String *fromString) {
String *self = malloc(sizeof(String));
self->data = malloc(fromString->size+ 1);
self->size = fromString->size;
self->length = fromString->length;
for(int i = 0;
i < fromString->length;
i ++)self->data[i]= fromString->data[i];
self->data[self->length]= 0;
return self;
}
String * _String_init_fromCString(char *fromCString) {
String *self = malloc(sizeof(String));
self->length = strlen(fromCString);
self->size = 2 * self->length;
self->data = malloc(self->size + 1);
for(int i = 0;
i < self->length;
i ++)self->data[i]= fromCString[i];
self->data[self->length]= 0;
return self;
}
void _String_resize(String *self) {
self->size *= 2;
self->data = realloc(self->data , self->size + 1);
}
void _String_resize_size(int size, String *self) {
self->size = size;
self->data = realloc(self->data , self->size + 1);
}
void _String_append_c(char c, String *self) {
if(self->length + 1 > self->size * _String_LOAD_FACTOR)_String_resize(self);
self->data[self->length ++]= c;
self->data[self->length]= 0;
}
char  _String_charAt_index(int index, String *self) {
if(index >= self->length)return - 1;
return self->data[index];
}
void _String_append_string(String *string, String *self) {
if(self->length + string->length> self->size * _String_LOAD_FACTOR){ 	const int t = 2 *(self->length + string->length);
_String_resize_size(t, self);
}
for(int i = self->length;
i < string->length+ self->length;
i ++)self->data[i]= _String_charAt_index(i, string);
self->length += string->length;
self->data[self->length]= 0;
}
void _String_append_cstring(char *cstring, String *self) {
	const int len = strlen(cstring);
if(self->length + len > self->size * _String_LOAD_FACTOR){ 	const int t = 2 *(self->length + len);
_String_resize_size(t, self);
}
for(int i = self->length;
i < self->length + len;
i ++)self->data[i]= cstring[i];
self->length += len;
self->data[self->length]= 0;
}
char * _String_cString(String *self) {
return self->data;
}
unsigned char  _String_equals_string(String *string, String *self) {
if(string->length= self->length)return 0;
for(int i = 0;
i < string->length;
i ++)if(string->data[i]= self->data[i])return 0;
return 1;
}

typedef struct {
	String **data;
	int count;
	int size;
} StringArray;
static const int _StringArray_DEFAULT_SIZE = 8;
static const double _StringArray_LOAD_FACTOR = 0.6;
StringArray * _StringArray_init() {
StringArray *self = malloc(sizeof(StringArray));
self->data = malloc(sizeof(String *)* _StringArray_DEFAULT_SIZE);
self->size = _StringArray_DEFAULT_SIZE;
self->count = 0;
return self;
}
StringArray * _StringArray_init_fromArray_length(String **fromArray, int length) {
StringArray *self = malloc(sizeof(StringArray));
self->size = length * 2;
self->data = malloc(sizeof(String *)* self->size);
self->count = length;
return self;
}
void _StringArray_deinit(StringArray *self) {
free(self->data);
free(self);
}
String * _StringArray_getValue_atIndex(int atIndex, StringArray *self) {
if(atIndex >= self->count)return - 1;
return self->data[atIndex];
}
void _StringArray_resize(StringArray *self) {
self->size *= 2;
self->data = realloc(sizeof(String *)* self->size);
}
void _StringArray_append_value(String *value, StringArray *self) {
if(self->count > _StringArray_LOAD_FACTOR * self->size){ _StringArray_resize(self);
}
self->data[self->count ++]= value;
}
String * _StringArray_pop(StringArray *self) {
return self->data[-- self->count];
}
String * _StringArray_remove_atIndex(int atIndex, StringArray *self) {
	const int result = self->data[atIndex];
for(int i = atIndex;
i < self->count - 1;
i ++)self->data[i]= self->data[i + 1];
return result;
}

typedef struct {
	String **keys;
	String **values;
	int count;
	int size;
} StringStringDict;
static const double _StringStringDict_LOAD_FACTOR = 0.6;
static const int _StringStringDict_DEFAULT_SIZE = 8;
static const int _StringStringDict_DUMMY_VALUE = - 1;
StringStringDict * _StringStringDict_init() {
StringStringDict *self = malloc(sizeof(StringStringDict));
self->keys = malloc(sizeof(String *)* _StringStringDict_DEFAULT_SIZE);
self->values = malloc(sizeof(String *)* _StringStringDict_DEFAULT_SIZE);
self->count = 0;
self->size = _StringStringDict_DEFAULT_SIZE;
return self;
}
void _StringStringDict_resize(StringStringDict *self) {
	const int oldSize = self->size;
self->size *= 2;
	String **newKeys = malloc(sizeof(String *)* self->size);
	String **newValues = malloc(sizeof(String *)* self->size);
for(int i = 0;
i < oldSize;
i ++){ 	int index = _StringStringDict_hash_string(forKey, self)% self->size;
while(newValues[index]){ index ++;
index = index % self->size;
}
newValues[index]= self->values[i];
newKeys[index]= self->keys[i];
}
free(self->values);
free(self->keys);
self->values = newValues;
self->keys = newKeys;
}
unsigned long  _StringStringDict_hash_string(String *string, StringStringDict *self) {
	unsigned long result = 7;
for(int i = 0;
i < string->length;
i ++)hash = hash * 31 + _String_charAt_index(i, string);
return result;
}
void _StringStringDict_insert_key_value(String *key, String *value, StringStringDict *self) {
if(self->count > self->size * _StringStringDict_LOAD_FACTOR)_StringStringDict_resize(self);
	int index = _StringStringDict_hash_string(key, self)% self->size;
while(self->values[index]){ index ++;
index = index % self->size;
}
self->keys[index]= key;
self->values[index]= value;
self->count ++;
}
void _StringStringDict_remove_key(String *key, StringStringDict *self) {
}
String * _StringStringDict_getValue_forKey(String *forKey, StringStringDict *self) {
	int index = _StringStringDict_hash_string(key, self)% self->size;
while(self->values[index]self->values[index]== _StringStringDict_DUMMY_VALUE){ index ++;
index = index % self->size;
}
return self->values[index];
}
unsigned char  _StringStringDict_contains_key(String *key, StringStringDict *self) {
	int index = _StringStringDict_hash_string(key, self)% self->size;
while(self->values[index]self->values[index]== _StringStringDict_DUMMY_VALUE){ index ++;
index = index % self->size;
}
}
