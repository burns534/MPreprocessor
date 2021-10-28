#include "lexer.h"

#define DEFAULT_USER_TYPES_LENGTH 128
#define MAX_TOKEN_NUM 4096
#define MAX_FUNCTION_NUM 1024
#define MAX_VARIABLE_NUM 1024
#define DEFAULT_BODY_SIZE 4096
#define MAX_CLASS_FUNCTIONS 128
#define MAX_CLASS_VARS 128
#define MAX_CLASS_NUM 64
#define MAX_FUNCTION_BODY_LENGTH 4096

#define MAX_CLASS_DEF_LENGTH 1 << 16
#define MAX_VARIABLE_DEC_LENGTH 256
#define MAX_FUNCTION_DEF_LENGTH 1 << 15
#define MAX_UDT_COUNT 256
#define MAX_SYMBOL_TABLE_COUNT 512
#define MAX_ARG_COUNT 16


typedef struct {
    char is_var, *identifier;
    Token *type;
} variable;

typedef struct {
    char *name;
    Token *type;
} argument;

typedef struct {
    char is_init, *name, *internal_name, *scope;
    argument **args;
    Token *returnType;
    Token **body;
    long arg_count, body_count;
} function;

typedef struct {
    char *name, *internal_name;
    long variable_count, function_count;
    variable **variables;
    function **functions;
} class;

void parse(const char *infile, const char *outfile);