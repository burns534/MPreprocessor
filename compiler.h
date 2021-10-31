#include "lexer.h"


#define MAX_TOKEN_NUM 1 << 16
// #define DEFAULT_BODY_SIZE 4096
// #define MAX_CLASS_FUNCTIONS 128
// #define MAX_CLASS_VARS 128
// #define MAX_CLASS_NUM 64
// #define MAX_FUNCTION_BODY_LENGTH 4096

#define MAX_CLASS_DEF_LENGTH 1 << 16
#define MAX_VARIABLE_DEC_LENGTH 256
#define MAX_FUNCTION_DEF_LENGTH 1 << 15
#define MAX_UDT_COUNT 256
#define MAX_SYMBOL_TABLE_COUNT 512
#define MAX_ARG_COUNT 16
#define MAX_STATIC_VARS 64

void compile(const char *, const char *, const char *);