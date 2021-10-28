#include "parser.h"
// static long user_defined_types_length = 0, class_table_count = 0, token_count = 0, cursor = 0;
// static char **user_defined_types, **user_defined_types_internal;
static long token_count = 0, cursor = 0, udt_count = 0, symbol_table_top = 0, symbol_top_stack_top = 0;
static char **udt_table, **symbol_table_keys, **symbol_table_values;
static long symbol_top_stack[64];
// static function **function_table;
// static variable **variable_table;
// static class **class_table;
static Token **tokens;

static inline char * append(char *str1, char *str2) {
    size_t len = strlen(str1) + strlen(str2) + 1;
    char *result = malloc(len);
    strcpy(result, str1);
    strcat(result, str2);
    result[len - 1] = 0;
    return result;
}

// linear search... don't want to implement set struct in C
static inline int array_contains_string(char **array, int array_length, char *string) {
    for (int i = 0; i < array_length; i++) {
        if (strcmp(array[i], string) == 0)
            return 1;
    }
    return 0;
}

static inline int array_contains_int(int *array, int array_length, int n) {
    for (int i = 0; i < array_length; i++) {
        if (array[i] == n)
            return 1;
    }
    return 0;
}

static inline char * type_name_for_symbol(char *symbol) {
    for (int i = 0; i < symbol_table_top; i++) {
        if (strcmp(symbol_table_keys[i], symbol) == 0)
            return symbol_table_values[i];
    }
    return NULL;
}

static inline int type_name(Token *token) {
    return array_contains_string(udt_table, udt_count, token->string);
}

static inline int type_name_string(char *identifier) {
    return array_contains_string(udt_table, udt_count, identifier);
}

static inline int type_specifier(Token *token) {
    // puts("type spec called");
    return array_contains_int((int *) type_specifiers, 6, token->type) || type_name(token);
}

// static char * generate_method_signature(char *class_name, char *function_name) {
//     size_t l1 = strlen(class_name), l2 = strlen(function_name);
//     char *result = malloc(l1 + l2 + 2);
//     memcpy(result, class_name, l1);
//     result[l1 + 1] = '_';
//     memcpy(result, function_name, l1 + 1);
//     result[l1 + l2 + 1] = 0;
//     return result;
// }

static char *generate_method_signature(char *class_name, char *function_name, char **arg_labels, int arg_count) {
    int l = arg_count + 3;
    for (int i = 0; i < arg_count; i++)
        l += strlen(arg_labels[i]);
    char *result;
    if (!class_name) {
        result = malloc(strlen(function_name) + l); // arg_count underscores plus 2 more plus null plus arg length
        sprintf(result, "_%s", function_name);
    } else {
        result = malloc(strlen(class_name) + strlen(function_name) + l); // arg_count underscores plus 2 more plus null plus arg length
        sprintf(result, "_%s_%s", class_name, function_name);
    }
    for (int i = 0; i < arg_count; i++) {
        result = append(result, "_");
        result = append(result, arg_labels[i]);
    }
    return result;
}

static int primary_expression(Token *token) {
    return token->type == IDENTIFIER || 
            token->type == STRING_LITERAL ||
            token->type == CONSTANT;
}

// static variable * variable_declaration() {
//     if (cursor + 4 > token_count) return NULL;
//     // puts("variable dec called");
//     variable *result = malloc(sizeof(variable));
//     // printf("cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
//     if (tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == COLON && type_specifier(tokens[cursor + 3])) {
//         if (tokens[cursor]->type == VAR) {
//             result->identifier = tokens[cursor + 1]->string;
//             result->type = tokens[cursor + 3];
//             result->is_var = 1;
//             cursor += 3;
//             return result;
//         } else if (tokens[cursor]->type == LET) {
//             result->identifier = tokens[cursor + 1]->string;
//             result->type = tokens[cursor + 3];
//             result->is_var = 0;
//             cursor += 3;
//             return result;
//         }
//     }
//     return NULL;
// }



// static argument * function_argument() {
//     if (tokens[cursor]->type == IDENTIFIER && tokens[cursor + 1]->type == COLON && type_specifier(tokens[cursor + 2])) {
//         argument *result = malloc(sizeof(argument));
//         result->name = tokens[cursor]->string;
//         result->type = tokens[cursor + 2];
//         cursor += 3;
//         return result;
//     } 
//     return NULL;
// }

// static class * allocate_class() {
//     class *result = malloc(sizeof(class));
//     result->functions = malloc(sizeof(function *) * MAX_CLASS_FUNCTIONS);
//     result->variables = malloc(sizeof(variable *) * MAX_CLASS_VARS);
//     return result;
// }

// static class * initialize_class(char *name) {
//     class *result = allocate_class();
//     result->function_count = result->variable_count = 0;
//     result->name = name;
//     result->internal_name = user_defined_types_internal[array_contains_string(user_defined_types, user_defined_types_length, result->name)];
//     return result;
// }

// static void deallocate_class(class *c) {
//     free(c->functions);
//     free(c->variables);
//     free(c);
// }

// static function * allocate_function() {
//     function *result = malloc(sizeof(function));
//     result->args = malloc(sizeof(argument *) * FUNCTION_MAX_ARGS);
//     result->body = malloc(sizeof(Token *) * DEFAULT_BODY_SIZE);
//     return result;
// }

// static function * initialize_function(char *name, char *class_name, int is_init) {
//     function *result = allocate_function();
//     result->name = name;
//     result->scope = class_name;
//     result->internal_name = generate_method_signature(class_name, name);
//     result->arg_count = result->body_count = 0;
//     result->is_init = is_init;
//     return result;
// }

// static void deallocate_function(function *f) {
//     free(f->args);
//     free(f);
// }

// rewrite this
// static function * function_definition(char *class_name) {
//     if (cursor + 4 > token_count) return NULL;
//     printf("function def called with cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
//     long temp_cursor = cursor;

//     // two cases
//     if (tokens[cursor]->type != FUNC && tokens[cursor]->type != INIT)
//         return NULL;

//     function *result;
    
//     if (tokens[cursor]->type == INIT) {
//         assert(tokens[cursor + 1]->type == OPEN_PAREN);
//         result = initialize_function("init", class_name, 1);
//         cursor += 2;
//     } else {
//         assert(tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == OPEN_PAREN);
//         result = initialize_function(tokens[cursor + 1]->string, class_name, 0);
//         cursor += 3;
//     }

//     printf("got name: %s\n", result->name);

//     argument *arg;

//      while (result->arg_count < FUNCTION_MAX_ARGS) {
//         if ((arg = function_argument()))
//             result->args[result->arg_count++] = arg;
//         if (tokens[cursor]->type == CLOSE_PAREN)
//             break;
//         else if (tokens[cursor]->type == COMMA)
//             cursor++;
//     }

//     cursor++; // flush out close paren

//     printf("arg_count: %ld\n", result->arg_count);
//     printf("current token: %s\n", tokens[cursor]->string);
//     // two options
//     // 1. no return symbol, function returns void
//     // 2. return symbol, expect return type
//     // get return type

//     if (tokens[cursor]->type != RETURN_SYMBOL) {
//         // void or init
//         // this means ill have to handle if it was an init when i do code generation
//         result->returnType = create_token(VOID, "void");
//         printf("type is void or init. cursor: %s\n", tokens[cursor]->string);
//     } else {
//         assert(tokens[cursor]->type == RETURN_SYMBOL);
//         assert(type_specifier(tokens[cursor + 1]));
//         result->returnType = tokens[cursor + 1];
//         cursor += 2;
//     }
//     assert(tokens[cursor++]->type == OPEN_CURL_BRACE);
//     // get body
//     int nest_depth = 1;
//     while (cursor < token_count && result->body_count < DEFAULT_BODY_SIZE) {
//         if (tokens[cursor]->type == OPEN_CURL_BRACE)
//             nest_depth++;
//         else if (tokens[cursor]->type == CLOSE_CURL_BRACE)
//             nest_depth--;
//         if (nest_depth == 0)
//             break;
//         result->body[result->body_count++] = tokens[cursor++];
//     }
//     printf("cursor: %s, cursorvalue; %ld\n", tokens[cursor]->string, cursor);
// // cursor will be on last curl brace 
//     return result;
// }

// static class * class_definition() {
//     if (tokens[cursor]->type != CLASS || tokens[cursor + 1]->type != IDENTIFIER || tokens[cursor + 2]->type != OPEN_CURL_BRACE) 
//         return NULL;
//     long cursor_save = cursor;
//     variable *var;
//     function *func;
//     class *result = initialize_class(tokens[cursor + 1]->string);

//     cursor += 3;
//     // currently class definitions only allow for function defs and variable decs
//     while (cursor < token_count) {
//         if ((var = variable_declaration())) {
//             result->variables[result->variable_count++] = var;
//             // printf("got variable named %s with type %s\n", var->identifier, var->type->string);
//         } else if ((func = function_definition(result->name))) { // provide class name
//             result->functions[result->function_count++] = func;
//             // function_table[function_table_count++] = func;
//             // printf("got a function named %s with body_count %ld\n", func->name, func->body_count);
//         } else if (tokens[cursor]->type == CLOSE_CURL_BRACE) { // class def is over
//             return result;
//         } else {
//             deallocate_class(result);
//             cursor = cursor_save;
//             return NULL;
//         }
//         cursor++;
//     }

//     return NULL;
// }

static void error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static char * variable_declaration() {
    if (cursor + 3 >= token_count) return NULL;
    // puts("variable dec called");
    char * result, *identifier, *type;
    // printf("cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
    if (tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == COLON) {
        if (!type_specifier(tokens[cursor + 3])) {
            char error_message[100];
            snprintf(error_message, 100, "Error: Invalid type specifier for identifier %s", identifier);
            error(error_message);
        }
        char *identifier = tokens[cursor + 1]->string;
        char *type = tokens[cursor + 3]->string;
// symbol table manip
        symbol_table_keys[symbol_table_top] = identifier;
        symbol_table_values[symbol_table_top++] = type;

        if (tokens[cursor]->type == VAR) {
            // if it is a class, then it needs to be a pointer
            if (type_name_string(type)) {
                result = malloc(strlen(identifier) + strlen(type) + 6); // * + tab + space + semicolon + newline + 1 = 6
                sprintf(result, "\t%s *%s;\n", type, identifier);
            } else {
                result = malloc(strlen(identifier) + strlen(type) + 5); // tab + space + semicolon + newline + 1 = 5
                sprintf(result, "\t%s %s;\n", type, identifier);
            }
        } else if (tokens[cursor]->type == LET) { // enforcing this is going to be complicated
            if (type_name_string(type)) {
                result = malloc(strlen(identifier) + strlen(type) + 11); // * + tab + const + space + semicolon + newline + 1= 11
                sprintf(result, "\t%s *%s;\n", type, identifier);
            } else {
                result = malloc(strlen(identifier) + strlen(type) + 10); // tab + const + space + semicolon + newline + 1 = 10
                sprintf(result, "\t%s %s;\n", type, identifier);
            }
        } else {
            char error_message[100];
            snprintf(error_message, 100, "Syntax Error: variable declaration: missing var or let keyword for identifier %s", identifier);
            error(error_message);
        }
        cursor += 4;
        return result;
    }
    return NULL;
}
// called with cursor at token following open paren
static char * create_function_call(char *scope, char* scope_var, char *identifier) {
    char * arg_labels[8 * MAX_ARG_COUNT],
    *args[8 * MAX_ARG_COUNT],
    *arg_string,
    *result,
    error_message[200];
    int arg_count = 0;

    cursor++; // skip open paren
    // collect args
    while (tokens[cursor]->type != CLOSE_PAREN && arg_count < MAX_ARG_COUNT) {
        if (tokens[cursor]->type != IDENTIFIER) {
            snprintf(error_message, 200, "Syntax Error: expected identifier. Instead found %s inside method call %s", tokens[cursor]->string, identifier);
            error(error_message);
        }
        
        if (tokens[cursor + 1]->type != COLON) {
            snprintf(error_message, 200, "Syntax Error: expected colon. instead found %s inside method call %s", tokens[cursor]->string, identifier);
            error(error_message);
        }
        
        // collect label and then collect string until comma
        arg_labels[arg_count] = tokens[cursor]->string;
        arg_string = "";
        cursor += 2; // skip colon
        while (tokens[cursor]->type != COMMA && tokens[cursor]->type != CLOSE_PAREN) {
            arg_string = append(arg_string, tokens[cursor]->string);
            cursor++;
        }
        args[arg_count++] = arg_string;

        if (tokens[cursor]->type == COMMA)
            cursor++;
    }
    
    result = generate_method_signature(scope, identifier, arg_labels, arg_count);

    result = append(result, "(");

    for (int i = 0; i < arg_count; i++) {
        result = append(result, args[i]);
        result = append(result, ", ");
    }

    if (scope && scope_var) {
        result = append(result, scope_var);
    }

    if (result[strlen(result)] == ' ')
        result[strlen(result) - 2] = 0; // ignore the last comma
    
    result = append(result, ")");
    cursor++; // increment past close paren
    return result;
}

static char * function_definition(char *scope) {
    if (cursor + 4 >= token_count) return NULL; // shortest possible func id()
    if (tokens[cursor]->type == FUNC) {
        if (tokens[cursor + 1]->type != IDENTIFIER) {
            printf("%d\n", tokens[cursor + 1]->type);
            puts(tokens[cursor + 1]->string);
            error("Syntax Error: Expected identifier following 'func' keyword");
        }
        const int e_length = 200;
        char error_message[e_length], 
        *result = "",
        *identifier = tokens[cursor + 1]->string,
        *arg_string = "(",
        **arg_labels = malloc(MAX_ARG_COUNT);
// SCOPE CHANGE
        symbol_top_stack[symbol_top_stack_top++] = symbol_table_top;

// symbol table manip 
// this feature won't work without storing separate table for each class
        // if (scope) {
        //     symbol_table_keys[symbol_table_top] = identifier;
        //     symbol_table_keys[symbol_table_top++] = scope;
        // }
    
        if (tokens[cursor + 2]->type != OPEN_PAREN) {
            snprintf(error_message, 200, "Syntax Error: Expected parenthesis following function name '%s'", identifier);
            error(error_message);
        }
        
        cursor += 3;
        int arg_count = 0;
        // get arguments
        while (cursor < token_count - 2 && tokens[cursor]->type != CLOSE_PAREN) {
            if (tokens[cursor]->type == IDENTIFIER) {
                char *arg_identifier = tokens[cursor]->string;
                if (tokens[cursor + 1]->type != COLON) {
                    snprintf(error_message, e_length, "Syntax Error: expected colon following identifier. params '%s'", identifier);
                    error(error_message);
                } else if (!type_specifier(tokens[cursor + 2])) {
                    snprintf(error_message, e_length, "Syntax Error: expected type specifier following identifier. params '%s'", identifier);
                    error(error_message);
                }
                char *arg_type = tokens[cursor + 2]->string, *arg;
                if (type_name_string(arg_type)) { // have to convert to pointer
                    arg = malloc(strlen(arg_identifier) + strlen(arg_type) + (arg_count > 0 ? 5 : 3)); // space + , + space + * + 1
                    sprintf(arg, arg_count > 0 ? ", %s *%s" : "%s *%s", arg_type, arg_identifier);
                } else {
                    arg = malloc(strlen(arg_identifier) + strlen(arg_type) + (arg_count > 0 ? 4 : 2)); // space + , + space + 1
                    sprintf(arg, arg_count > 0 ? ", %s %s" : "%s %s", arg_type, arg_identifier);
                }
                arg_string = append(arg_string, arg);
                arg_labels[arg_count++] = arg_identifier;
 // symbol table here               
                symbol_table_keys[symbol_table_top] = arg_identifier;
                symbol_table_values[symbol_table_top++] = arg_type;

                cursor += 3;
                continue;
            }
            cursor++;
        }
        if (scope) {
            if (arg_count > 0)
                arg_string = append(arg_string, ", ");
            char *temp = malloc(strlen(scope) + 7);
            sprintf(temp, "%s *self", scope);
            arg_string = append(arg_string, temp);
        }
        
        // now two cases, assume void or explicit return type

        if (tokens[cursor + 1]->type == RETURN_SYMBOL) {
            if (!type_specifier(tokens[cursor + 2])) {
                snprintf(error_message, e_length, "Syntax Error: Expected type specifier following return annotation. Function '%s'", identifier);
                error(error_message);
            }
            result = tokens[cursor + 2]->string;
            result = append(result, " ");
            if (type_name(tokens[cursor + 2]))
                result = append(result, " * ");
            cursor += 4;
        } else {
            result = "void ";
            cursor += 2;
        }

        result = append(result, generate_method_signature(scope, identifier, arg_labels, arg_count)); // FIXME need to generate unique name

        result = append(result, arg_string);

        result = append(result, ") {\n");

        // handle function body
        int bracket_depth = 1;

        // if a segmentation fault happens in here, somebody did bad brackets
        while (cursor < token_count && bracket_depth > 0) {
            if (tokens[cursor]->type == OPEN_CURL_BRACE) {
                bracket_depth++;
// SCOPE CHANGE
                symbol_top_stack[symbol_top_stack_top++] = symbol_table_top;
            } else if (tokens[cursor]->type == CLOSE_CURL_BRACE) {
                bracket_depth--;
// SCOPE CHANGE
                if (bracket_depth > 0) // because we started bracket_depth at 1
                    symbol_table_top = symbol_top_stack[--symbol_top_stack_top];
            } else if (type_name(tokens[cursor]) && tokens[cursor + 1]->type == IDENTIFIER) {
// symbol table manip
                symbol_table_keys[symbol_table_top] = tokens[cursor + 1]->string;
                symbol_table_values[symbol_table_top++] = tokens[cursor]->string;
                cursor += 2;
                continue;
            } else if (tokens[cursor]->type == SELF) {
                if (tokens[cursor + 1]->type == DOT) {
                    if (tokens[cursor + 2]->type != IDENTIFIER)
                        error("Syntax Error");
                    char *temp_identifier = tokens[cursor + 2]->string;
                    // two cases: var or func
                    if (tokens[cursor + 3]->type == OPEN_PAREN) { // func case 2
                        // gather function params and their types
                        cursor += 4;
                        result = append(result, create_function_call(scope, "self", temp_identifier));
                        continue;
                    } else { // variable case 3
                        result = append(result, "self");
                        result = append(result, "->");
                        result = append(result, temp_identifier);
                        cursor += 3;
                        continue;
                    }
                }
            } else if (tokens[cursor]->type == INIT) {
                if (tokens[cursor + 1]->type != OPEN_PAREN)
                    error("Syntax Error");
                cursor += 2;
                result = append(result, create_function_call(scope, NULL, tokens[cursor]->string));
                continue;
            } else if (tokens[cursor]->type == IDENTIFIER && tokens[cursor + 1]->type == DOT && tokens[cursor + 2]->type == IDENTIFIER) {
                // 2 cases
                // case 1 varid.funcid
                char *typename, 
                *variable_identifier = tokens[cursor]->string,
                *second_identifier = tokens[cursor + 2]->string;
                if (!(typename = type_name_for_symbol(tokens[cursor]->string))) {
                    error("Error: dot operator on invalid typename");
                }

                if (tokens[cursor + 3]->type == OPEN_PAREN) {
                    cursor += 4;
                    result = append(result, create_function_call(typename, variable_identifier, second_identifier));
                    continue;
                } else {
                    result = append(result, variable_identifier);
                    result = append(result, "->");
                    result = append(result, second_identifier);
                    cursor += 3;
                    continue;
                }  
            }
            result = append(result, tokens[cursor]->string);
            if (tokens[cursor]->type == COMMA || tokens[cursor]->type == CLOSE_CURL_BRACE) {
                result = append(result, "\n");
            } else {
                result = append(result, " ");
            }
            cursor++;

        }
// SCOPE CHANGE
        symbol_table_top = symbol_top_stack[--symbol_top_stack_top];

        return result;
    }
    return NULL;
}

static char * class_definition() {
    if (cursor + 3 >= token_count || tokens[cursor]->type != CLASS) // minimum is class id {}
        return NULL;
    else if (tokens[cursor + 1]->type != IDENTIFIER) 
        error("Syntax Error: expected identifier in class declaration");
    else if (tokens[cursor + 2]->type != OPEN_CURL_BRACE)
        error("Syntax Error: expected open bracket for class declaration");
// SCOPE CHANGE
    symbol_top_stack[symbol_top_stack_top++] = symbol_table_top; // save cursor on stack

    char *struct_def = "\ntypedef struct {\n", 
    *method_defs = "", 
    *new_item = "", 
    *identifier = tokens[cursor + 1]->string,
    *end = malloc(4 + strlen(identifier)); // close + semicolon + id + space + nl
    sprintf(end, "} %s;\n", identifier);
    // gather symbol
    udt_table[udt_count++] = identifier;

    cursor += 3;
    // at this point, we should be inside the open curly bracket
    while (cursor < token_count && tokens[cursor]->type != CLOSE_CURL_BRACE) {
        // try to get variable
        if ((new_item = variable_declaration())) {
            struct_def = append(struct_def, new_item);
        } else if ((new_item = function_definition(identifier))) {
            method_defs = append(method_defs, new_item);
        } else {
            cursor++;
        }
        // try to get function def
    }
    cursor++; // close curl bracket

// SCOPE CHANGE
    symbol_table_top = symbol_top_stack[--symbol_top_stack_top];
    // printf("cursor: %ld, token_count: %ld\n", cursor, token_count);
    struct_def = append(struct_def, end);
    method_defs = append(method_defs, struct_def);
    return method_defs;
}
// populate user-defined types table

// static void first_pass() {
//     while (cursor < token_count - 1) {
//         if (tokens[cursor]->type == CLASS) {
//             assert(tokens[cursor + 1]->type == IDENTIFIER);
//             user_defined_types[user_defined_types_length] = tokens[cursor + 1]->string;
//             user_defined_types_internal[user_defined_types_length++] = generate_class_signature(tokens[cursor + 1]->string);
//             cursor += 4; // at least 4 ahead because smallest allowed would be class id {}
//             continue;
//         }
//         cursor++;
//     }
//     puts("----user-defined types-----");
//     for (int i = 0; i < user_defined_types_length; i++) {
//         printf("%s\n", user_defined_types[i]);
//     }
//     cursor = 0;
// }



static char * first_pass() {
    char *result = "", *temp;
    while (cursor < token_count) {
        if ((temp = class_definition())) {
            result = append(result, temp);
        } else if ((temp = function_definition(NULL))) {
            result = append(result, temp);
        } else {
            cursor++;
        }
    }
    return result;
}

// static void second_pass() {
//     variable *var;
//     class *class_def;
//     function *func;
//     while(cursor < token_count) {
//         if ((class_def = class_definition())) {
//             printf("got class def with name: %s\n", class_def->name);
//         }
//         cursor++;
//     }
//     cursor = 0;
// }

/*
so we need to generate a symbol table first
normal code generator - needs to know its scope i.e. its typename owner
so it knows what self means

will add generic support in next iteration


tables I need:
<--------------->
udt table (typename -> udt) (check containment)
symbol table (identifier -> udt) (check containment) (generated locally)
classname -> variable (check containment)
classname -> user declared fuction name (check containment)
*/

/* Example
    // class Dictionary maps to 
    typedef struct {
        int *data, size, count;
    } Dictionary;
    // needs a symbol table for attributes and methods, constructed first pass
    // function definitions inside class are mapped to unique generated name
    // and defined statically in the c file
    // func contains(value: int) -> bool maps to bool contains(Dictionary *self, int value)
    // just like Python explicit declarations

    class Dictionary {
        int *data, size, count;
        // Before generating the body, I need to know the scope I'm in
        // so each function definition object needs its owner name in the lookup table

        // so we need to iterate through the code, if I encounter an identifier
        // which is in my lookup table, I need to replace it appropriately

        // 1. case: variable (class owned)
        // self->IDENTIFIER 

        // 2. case: function (class owned)
        // generated_function_name(self, params)
        // might skip 3

        // 3. case: stack defined variable of udt
        // Stack stack; yields
        // Stack stack; 
        // stack_init_name(&stack);

// for now, not allowing pointers
        // 4. case: dynamically defined variable of udt
        // List list = List(length: 4) yields 
        // List *list = malloc(sizeof(List)); 
        // list_init_name(list, 4);

        // for the following two cases, I need to parse the expression,
        // to get the type, for this I need a symbol table..
        // using symbol table, I check the semantics of the function
        // and generate the appropriate signature and fill in the params

        // 5. case: . operator function call
        // stack.push(4) yields
        // _push_signature(&stack, 4);
// wont allow pointers for now
        // 6. case: -> operator function call
        // list->append(4); yields
        // _append_signature(list, 4);

        func contains(value: int) -> bool {
            int current = 0;
            Stack stack;
            stack.push(4);
            List list = List(length: 4);
            list->append(4);
            while (current >= 0) {
                current = hash(current);
                if (data[current] == value) {
                    return true;
                }
            }
            return false;
        }

        int hash(int value) { return magic number; }
    }
*/


// static variable * initialize_variable(char *identifier, Token *type) {
//     variable *result = malloc(sizeof(variable));
//     result->identifier = identifier;
//     result->type = type;
//     result->is_var = 1;
//     return result;
// }

// will perform extremely limited semantic analysis for version 0
// char * generate_function_body(function *f) {
//     int cursor = 0, variable_count = 0, result_cursor = 0;
//     char *result = malloc(MAX_FUNCTION_BODY_LENGTH);
//     // local symbol table
//     variable **vars = malloc(sizeof(variable) * MAX_VARIABLE_NUM);
//     // add the function arguments to the local symbol table
//     while (variable_count < f->arg_count) {
//         vars[variable_count] = initialize_variable(f->args[variable_count]->name, f->args[variable_count]->type);
//     }
//     // finish populating local symbol table and look for definitions
//     while (cursor < f->body_count) {
//         if (type_name(f->body[cursor]->type) && f->body[cursor + 1]->type == IDENTIFIER) {
//             vars[variable_count++] = initialize_variable(f->body[cursor]->string, f->body[cursor + 1]->string);
//             cursor += 2;
//             continue;
//         }
//         cursor++;
//     }

//     cursor = 0;
//     // do the cases
//     int class_index = get_class_index(f->scope);
//     char *function_signature;
//     while (cursor < f->body_count - 3) {
//         if (f->body[cursor + 1]->type == DOT) {
//             if (f->body[cursor]->type == SELF && ftable_contains(class_table[class_index], f->body[cursor + 2]->string)) {
//                 function_signature = generate_method_signature(f->scope, f->body[cursor + 2]->string);
//             } else if (f->body[cursor]->type == IDENTIFIER) {
//                 int index;
//                 if ((index = get_var_index(vars, variable_count, f->body[cursor]->string)) != -1) {
                    
//                 } else {
//                     perror("variable not associated with udt");
//                 }
//                 char *scope = class_table[get_class_index(f->body[cursor]->string)]; // this will cause seg fault if bad variable name
//                 ftable_contains(scope, f->body[cursor + 2]->string);
//                 function_signature = generate_method_signature(scope, f->body[cursor + 2]->string);
//             } else {
//                 continue;
//             }
//             // case 5, generate func signature
//             strcpy(result + result_cursor, function_signature);
//             result_cursor += strlen(function_signature);
//             result[result_cursor + 1] = '(';
//             strcpy(result + result_cursor + 1, "self");
//             if (f->body[cursor + 4]->type == CLOSE_PAREN) {
//                 // no params
//                 result[result_cursor + 6] = ')';
//             } else {
//                 // multiple params
//                 result[result_cursor + 6] = ',';
//             }
//             cursor += 4;
//             continue;
//         } 
//     }

//     return result;
// }


void parse(const char *infile, const char *outfile) {
    FILE *input = fopen(infile, "r"), *output = fopen(outfile, "w");
    // user-defined types
    // user_defined_types_length = 0;
    // user_defined_types = malloc(sizeof(char *) * DEFAULT_USER_TYPES_LENGTH);
    // user_defined_types_internal = malloc(sizeof(char *) * DEFAULT_USER_TYPES_LENGTH);

    tokens = malloc(sizeof(Token *) * MAX_TOKEN_NUM);
    token_count = tokenize(infile, tokens, MAX_TOKEN_NUM);

    udt_table = malloc(8 * MAX_UDT_COUNT);
    symbol_table_keys = malloc(8 * MAX_SYMBOL_TABLE_COUNT);
    symbol_table_values = malloc(8 * MAX_SYMBOL_TABLE_COUNT);

    // printf("token count: %ld\n", token_count);
    // for (int i = 0; i < token_count; i++) {
    //     printf("index: %d, type: %d, value: %s\n", i, tokens[i]->type, tokens[i]->string);
    // }

    // class_table = malloc(sizeof(class *) * MAX_CLASS_NUM);
    // for (int i = 0; i < token_count; i++) {
    //     puts(tokens[i]->string);
    // }
    fputs(first_pass(), output);

    // int bracket_depth = 0;

    // while(cursor < token_count) {
    //     if (tokens[cursor]->type == CLASS) {
    //         bracket_depth++;

    //     }
    // }
}