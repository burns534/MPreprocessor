#include <stdlib.h>

typedef struct {
	int *data;
	int count;
	int size;
} IntArray;
IntArray * _IntArray_init();
IntArray * _IntArray_init_fromArray_length(int *fromArray, int length);
void _IntArray_deinit(IntArray *self);
int  _IntArray_getValue_atIndex(int atIndex, IntArray *self);
void _IntArray_resize(IntArray *self);
void _IntArray_append_value(int value, IntArray *self);
int  _IntArray_pop(IntArray *self);
int  _IntArray_remove_atIndex(int atIndex, IntArray *self);

typedef struct {
	char *data;
	int length;
	int size;
} String;
String * _String_init();
String * _String_init_fromString(String *fromString);
String * _String_init_fromCString(char *fromCString);
void _String_resize(String *self);
void _String_resize_size(int size, String *self);
void _String_append_c(char c, String *self);
char  _String_charAt_index(int index, String *self);
void _String_append_string(String *string, String *self);
void _String_append_cstring(char *cstring, String *self);
char * _String_cString(String *self);
unsigned char  _String_equals_string(String *string, String *self);
void _String_print(String *self);

typedef struct {
	String **data;
	int count;
	int size;
} StringArray;
StringArray * _StringArray_init();
StringArray * _StringArray_init_fromArray_length(String **fromArray, int length);
void _StringArray_deinit(StringArray *self);
String * _StringArray_getValue_atIndex(int atIndex, StringArray *self);
void _StringArray_resize(StringArray *self);
void _StringArray_append_value(String *value, StringArray *self);
String * _StringArray_pop(StringArray *self);
String * _StringArray_remove_atIndex(int atIndex, StringArray *self);

typedef struct {
	int type;
	String *string;
} Token;
Token * _Token_init();
Token * _Token_init_type_string(int type, String *string);

typedef struct {
	Token **data;
	int count;
	int size;
} TokenArray;
TokenArray * _TokenArray_init();
TokenArray * _TokenArray_init_fromArray_length(String **fromArray, int length);
void _TokenArray_deinit(TokenArray *self);
Token * _TokenArray_getValue_atIndex(int atIndex, TokenArray *self);
void _TokenArray_resize(TokenArray *self);
void _TokenArray_append_value(Token *value, TokenArray *self);
Token * _TokenArray_pop(TokenArray *self);
Token * _TokenArray_remove_atIndex(int atIndex, TokenArray *self);

typedef struct {
	int cursor;
	String *input;
	TokenArray *tokens;
} Lexer;
Lexer * _Lexer_init();
unsigned char  _Lexer_inline_space(Lexer *self);
unsigned char  _Lexer_line_break(Lexer *self);
unsigned char  _Lexer_whitespace(Lexer *self);
void _Lexer_lex_filename(char *filename, Lexer *self);

typedef struct {
} Parser;
Parser * _Parser_init();
void _Parser_parse_filename(char *filename, Parser *self);
void _Parser_line_break(Parser *self);
