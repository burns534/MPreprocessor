#include "parser.h"
static long user_defined_types_length, function_names_length, tokens_length, cursor = 0;
static char **user_defined_types, **function_names;
static Token **tokens;

// linear search... don't want to implement set struct in C
static int array_contains(char **array, int array_length, char *string) {
    for (int i = 0; i < array_length; i++) {
        if (strcmp(array[i], string) == 0)
            return 1;
    }
    return 0;
}

static int array_contains(int *array, int array_length, int n) {
    for (int i = 0; i < array_length; i++) {
        if (array[i] == n)
            return 1;
    }
    return 0;
}

static int type_name(Token *token) {
    return array_contains(user_defined_types, user_defined_types_length, token->string);
}

static int type_specifier(Token *token) {
    return array_contains(type_specifiers, 6, token->type) || type_name(token);
}

static int primary_expression(Token *token) {
    return token->type == IDENTIFIER || 
            token->type == STRING_LITERAL ||
            token->type == CONSTANT;
}

static variable_declaration * get_variable_declaration() {
    if (tokens[cursor]->type == VAR) {

    }
    return NULL;
}

void parse(const char *infile, const char *outfile) {
    FILE *input = fopen(infile, "r"), *output = fopen(outfile, "w");
    // user-defined types
    user_defined_types_length = DEFAULT_USER_TYPES_LENGTH;
    user_defined_types = malloc(sizeof(char *) * DEFAULT_USER_TYPES_LENGTH);

    tokenize(infile, tokens, MAX_TOKEN_NUM * sizeof(Token *));

}