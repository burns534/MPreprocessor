#include "lexer.h"

#define DEFAULT_USER_TYPES_LENGTH 128
#define MAX_TOKEN_NUM 4096
#define LET 0
#define VAR 1

typedef struct {
    char type, *identifier;
}variable_declaration;

void parse(const char *infile, const char *outfile);