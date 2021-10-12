#include "parser.h"
static long user_defined_types_length, function_table_count = 0, token_count, cursor = 0;
static char **user_defined_types, **user_defined_types_internal;
static function **function_table;
static Token **tokens;

// linear search... don't want to implement set struct in C
static int array_contains_string(char **array, int array_length, char *string) {
    for (int i = 0; i < array_length; i++) {
        if (strcmp(array[i], string) == 0)
            return 1;
    }
    return 0;
}

static int array_contains_int(int *array, int array_length, int n) {
    for (int i = 0; i < array_length; i++) {
        if (array[i] == n)
            return 1;
    }
    return 0;
}

static int type_name(Token *token) {
    return array_contains_string(user_defined_types, user_defined_types_length, token->string);
}

static int type_specifier(Token *token) {
    // puts("type spec called");
    return array_contains_int((int *) type_specifiers, 6, token->type) || type_name(token);
}

static char * generate_method_signature(char *class_name, char *function_name) {
    size_t l1 = strlen(class_name), l2 = strlen(function_name);
    char *result = malloc(l1 + l2 + 2);
    memcpy(result, class_name, l1);
    result[l1 + 1] = '_';
    memcpy(result, function_name, l1 + 1);
    result[l1 + l2 + 1] = 0;
    return result;
}

static char * generate_class_signature(char *class_name) {
    return class_name;
}

static int primary_expression(Token *token) {
    return token->type == IDENTIFIER || 
            token->type == STRING_LITERAL ||
            token->type == CONSTANT;
}

static variable * variable_declaration() {
    if (cursor + 4 > token_count) return NULL;
    // puts("variable dec called");
    variable *result = malloc(sizeof(variable));
    // printf("cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
    if (tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == COLON && type_specifier(tokens[cursor + 3])) {
        if (tokens[cursor]->type == VAR) {
            result->identifier = tokens[cursor + 1]->string;
            result->type = tokens[cursor + 3];
            result->is_var = 1;
            cursor += 3;
            return result;
        } else if (tokens[cursor]->type == LET) {
            result->identifier = tokens[cursor + 1]->string;
            result->type = tokens[cursor + 3];
            result->is_var = 0;
            cursor += 3;
            return result;
        }
    }
    return NULL;
}

static argument * function_argument() {
    if (tokens[cursor]->type == IDENTIFIER && tokens[cursor + 1]->type == COLON && type_specifier(tokens[cursor + 2])) {
        argument *result = malloc(sizeof(argument));
        result->name = tokens[cursor]->string;
        result->type = tokens[cursor + 2];
        cursor += 3;
        return result;
    } 
    return NULL;
}

static class * allocate_class() {
    class *result = malloc(sizeof(class));
    result->functions = malloc(sizeof(function *) * MAX_CLASS_FUNCTIONS);
    result->variables = malloc(sizeof(variable *) * MAX_CLASS_VARS);
    return result;
}

static class * initialize_class(char *name) {
    class *result = allocate_class();
    result->function_count = result->variable_count = 0;
    result->name = name;
    result->internal_name = user_defined_types_internal[array_contains_string(user_defined_types, user_defined_types_length, result->name)];
    return result;
}

static void deallocate_class(class *c) {
    free(c->functions);
    free(c->variables);
    free(c);
}

static function * allocate_function() {
    function *result = malloc(sizeof(function));
    result->args = malloc(sizeof(argument *) * FUNCTION_MAX_ARGS);
    result->body = malloc(sizeof(Token *) * DEFAULT_BODY_SIZE);
    return result;
}

static function * initialize_function(char *name, char *class_name, int is_init) {
    function *result = allocate_function();
    result->name = name;
    result->internal_name = generate_method_signature(class_name, name);
    result->arg_count = result->body_count = 0;
    result->is_init = is_init;
    return result;
}

static void deallocate_function(function *f) {
    free(f->args);
    free(f);
}

// rewrite this
static function * function_definition(char *class_name) {
    if (cursor + 4 > token_count) return NULL;
    printf("function def called with cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
    long temp_cursor = cursor;

    // two cases
    if (tokens[cursor]->type != FUNC && tokens[cursor]->type != INIT)
        return NULL;

    function *result;
    
    if (tokens[cursor]->type == INIT) {
        assert(tokens[cursor + 1]->type == OPEN_PAREN);
        result = initialize_function("init", class_name, 1);
        cursor += 2;
    } else {
        assert(tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == OPEN_PAREN);
        result = initialize_function(tokens[cursor + 1]->string, class_name, 0);
        cursor += 3;
    }

    printf("got name: %s\n", result->name);

    argument *arg;

     while (result->arg_count < FUNCTION_MAX_ARGS) {
        if ((arg = function_argument()))
            result->args[result->arg_count++] = arg;
        if (tokens[cursor]->type == CLOSE_PAREN)
            break;
        else if (tokens[cursor]->type == COMMA)
            cursor++;
    }

    cursor++; // flush out close paren

    printf("arg_count: %ld\n", result->arg_count);
    printf("current token: %s\n", tokens[cursor]->string);
    // two options
    // 1. no return symbol, function returns void
    // 2. return symbol, expect return type
    // get return type

    if (tokens[cursor]->type != RETURN_SYMBOL) {
        // void or init
// FIXME: if init need to handle that here
        result->returnType = create_token(VOID, "void");
        printf("type is void or init. cursor: %s\n", tokens[cursor]->string);
    } else {
        assert(tokens[cursor]->type == RETURN_SYMBOL);
        assert(type_specifier(tokens[cursor + 1]));
        result->returnType = tokens[cursor + 1];
        cursor += 2;
    }
    assert(tokens[cursor++]->type == OPEN_CURL_BRACE);
    // get body
    int nest_depth = 1;
    while (cursor < token_count && result->body_count < DEFAULT_BODY_SIZE) {
        if (tokens[cursor]->type == OPEN_CURL_BRACE)
            nest_depth++;
        else if (tokens[cursor]->type == CLOSE_CURL_BRACE)
            nest_depth--;
        if (nest_depth == 0)
            break;
        result->body[result->body_count++] = tokens[cursor++];
    }
    printf("cursor: %s, cursorvalue; %ld\n", tokens[cursor]->string, cursor);
// cursor will be on last curl brace 
    return result;
}

static class * class_definition() {
    if (tokens[cursor]->type != CLASS || tokens[cursor + 1]->type != IDENTIFIER || tokens[cursor + 2]->type != OPEN_CURL_BRACE) 
        return NULL;
    long cursor_save = cursor;
    variable *var;
    function *func;
    class *result = initialize_class(tokens[cursor + 1]->string);

    cursor += 3;
    // currently class definitions only allow for function defs and variable decs
    while (cursor < token_count) {
        if ((var = variable_declaration())) {
            result->variables[result->variable_count++] = var;
            printf("got variable named %s with type %s\n", var->identifier, var->type->string);
        } else if ((func = function_definition(result->name))) { // provide class name
            result->functions[result->function_count++] = func;
            function_table[function_table_count++] = func;
            printf("got a function named %s with body_count %ld\n", func->name, func->body_count);
        } else if (tokens[cursor]->type == CLOSE_CURL_BRACE) { // class def is over
            return result;
        } else {
            deallocate_class(result);
            cursor = cursor_save;
            return NULL;
        }
        cursor++;
    }

    return NULL;
}
// populate user-defined types table
static void first_pass() {
    while (cursor < token_count - 1) {
        if (tokens[cursor]->type == CLASS) {
            assert(tokens[cursor + 1]->type == IDENTIFIER);
            user_defined_types[user_defined_types_length] = tokens[cursor + 1]->string;
            user_defined_types_internal[user_defined_types_length++] = generate_class_signature(tokens[cursor + 1]->string);
            cursor += 4; // at least 4 ahead because smallest allowed would be class id {}
            continue;
        }
        cursor++;
    }
    puts("----user-defined types-----");
    for (int i = 0; i < user_defined_types_length; i++) {
        printf("%s\n", user_defined_types[i]);
    }
    cursor = 0;
}

static void second_pass() {
    variable *var;
    class *class_def;
    while(cursor < token_count) {
        if ((class_def = class_definition())) {
            printf("got class def with name: %s\n", class_def->name);
        }
        cursor++;
    }
    cursor = 0;
}

void parse(const char *infile, const char *outfile) {
    FILE *input = fopen(infile, "r"), *output = fopen(outfile, "w");
    // user-defined types
    user_defined_types_length = 0;
    user_defined_types = malloc(sizeof(char *) * DEFAULT_USER_TYPES_LENGTH);
    user_defined_types_internal = malloc(sizeof(char *) * DEFAULT_USER_TYPES_LENGTH);

    tokens = malloc(sizeof(Token *) * MAX_TOKEN_NUM);
    token_count = tokenize(infile, tokens, MAX_TOKEN_NUM);

    // printf("token count: %ld\n", token_count);
    // for (int i = 0; i < token_count; i++) {
    //     printf("index: %d, type: %d, value: %s\n", i, tokens[i]->type, tokens[i]->string);
    // }

    function_table = malloc(sizeof(function *) * MAX_FUNCTION_NUM);

    first_pass();
    second_pass();

    for (int i = 0; i < function_table_count; i++) {
        printf("name: %s\n", function_table[i]->name);
    }

    int bracket_depth = 0;

    // while(cursor < token_count) {
    //     if (tokens[cursor]->type == CLASS) {
    //         bracket_depth++;

    //     }
    // }
}