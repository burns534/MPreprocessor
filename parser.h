#include "lexer.h"
#include <assert.h>

#define DEFAULT_USER_TYPES_LENGTH 128
#define MAX_TOKEN_NUM 4096
#define MAX_FUNCTION_NUM 1024
#define DEFAULT_BODY_SIZE 4096
#define MAX_CLASS_FUNCTIONS 128
#define MAX_CLASS_VARS 128
#define FUNCTION_MAX_ARGS 5

typedef struct {
    char is_var, *identifier;
    Token *type;
} variable;

typedef struct {
    char *name;
    Token *type;
} argument;

typedef struct {
    char is_init, *name, *internal_name;
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