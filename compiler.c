#include "compiler.h"
// static long user_defined_types_length = 0, class_table_count = 0, token_count = 0, cursor = 0;
// static char **user_defined_types, **user_defined_types_internal;
static long token_count = 0, cursor = 0, udt_count = 0, symbol_table_top = 0, symbol_top_stack_top = 0, static_vars_count = 0;
static char **udt_table, **symbol_table_keys, **symbol_table_values, **static_vars;
static long symbol_top_stack[64];
// static function **function_table;
// static variable **variable_table;
// static class **class_table;
static Token **tokens;

static inline void error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
} 

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
    // printf("type_name called with %s\n", token->string);
    return array_contains_string(udt_table, udt_count, token->string);
}

static inline int type_name_string(char *identifier) {
    return array_contains_string(udt_table, udt_count, identifier);
}

static inline int type_specifier(Token *token) {
    return array_contains_int((int *) type_specifiers, 8, token->type) || type_name(token);
}

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

static char * generate_static_name(char *scope, char *identifier) {
    char *result = "", temp[100];
    snprintf(temp, 100, "_%s_%s", scope, identifier);
    result = append(result, temp);
    return result;
}
// this is bad but it will have to do for now
static char * static_variable_definition(char *scope) { // shortest is static let identifier: type = value
    if (cursor + 6 >= token_count) return NULL;
    puts("static def");
    if (tokens[cursor]->type == STATIC) {
        if (tokens[cursor + 1]->type != LET && tokens[cursor + 1]->type != VAR) 
            error("Expected var or let qualifier");
        
        if (tokens[cursor + 2]->type != IDENTIFIER)
            error("Expected identifier");
        
        if (tokens[cursor + 3]->type != COLON)
            error("Expected colon");

        if (!type_specifier(tokens[cursor + 4]))
            error("Expected type specifier");

        if (tokens[cursor + 5]->type != EQ)
            error("Expected assignment in variable definition");
        

        char *identifier = tokens[cursor + 2]->string,
        temp[200],
        *name = generate_static_name(scope, identifier),
        *type = tokens[cursor + 4]->string,
        *value = "",
        *result = tokens[cursor + 1]->type == LET ? "static const " : "static ";
        cursor += 6;
        // add to static table
        static_vars[static_vars_count++] = identifier;

        // if you omit semicolon, this would be problematic
        while (tokens[cursor]->type != SEMICOLON) {
            value = append(value, tokens[cursor++]->string);
            value = append(value, " ");
        }

        value[strlen(value) - 1] = 0; // remove trailing space;

        snprintf(temp, 200, type_name_string(type) ? "%s *%s = %s;\n" : "%s %s = %s;\n", type, name, value);

        result = append(result, temp);
        cursor++; // skip semicolon
        return result;
    }
    return NULL;
}

static char * variable_declaration() {
    if (cursor + 3 >= token_count) return NULL;
    // printf("variable declaration %s\n", tokens[cursor + 1]->string);
    // puts("variable dec called");
    char *identifier, *type, *result = "";
    // printf("cursor: %s, +1: %s, +2: %s, +3: %s\n", tokens[cursor]->string, tokens[cursor + 1]->string, tokens[cursor + 2]->string, tokens[cursor + 3]->string);
    if (tokens[cursor + 1]->type == IDENTIFIER && tokens[cursor + 2]->type == COLON) {
        char *identifier = tokens[cursor + 1]->string,
        *type;
        result = append(result, tokens[cursor]->type == VAR ? "\t" : "\tconst ");
        int increment;
        // array dec
        if (tokens[cursor + 3]->type == OPEN_BRACE) {
            if (!type_specifier(tokens[cursor + 4]))
                error("Expected type specifier");
            else if (tokens[cursor + 5]->type != CLOSE_BRACE)
                error("Expected close bracket");
            type = tokens[cursor + 4]->string;
            result = append(result, type);
            result = append(result, " ");
            result = append(result, "*");
            increment = 6;
        } else {
            if (!type_specifier(tokens[cursor + 3]))
                error("Expected type specifier");
            type = tokens[cursor + 3]->string;
            result = append(result, type);
            result = append(result, " ");
            increment = 4;
        }

        if (type_name_string(type))
            result = append(result, "*");

        result = append(result, identifier);
        result = append(result, ";\n");

        
// symbol table manip
        printf("s top: %ld\n", symbol_table_top);
        symbol_table_keys[symbol_table_top] = identifier;
        symbol_table_values[symbol_table_top++] = type;

       
        cursor += increment;
        return result;
    }
    return NULL;
}
// called with cursor at token following open paren
static char * create_function_call(char *scope, char* scope_var, char *identifier) {
    // puts("create function call");
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
// does not support id.id.id
static char * gather_body(char *result, char *scope) {
    // handle function body
    int bracket_depth = 1;
    char *temp;
    // printf("gather body called at token: %s\n", tokens[cursor]->string);
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
        } else if (tokens[cursor]->type == VAR || tokens[cursor]->type == LET) { // init definition call
            if (tokens[cursor + 1]->type != IDENTIFIER)
                error("Syntax Error: expected identifier");
            temp = variable_declaration(); // get the string and update symbol table
            temp[strlen(temp) - 2] = 0; // remove the semicolon
            result = append(result, temp);
            
            if (tokens[cursor]->type != EQ) {
                continue; // valid
            }

            if (!type_name(tokens[cursor + 1])) {
                // not init call...
                result = append(result, " = ");
                cursor++; // advance past eq sign
                continue;
            }
            if (tokens[cursor + 2]->type != OPEN_PAREN)
                error("Syntax Error: expected parenthesis following type name");
            result = append(result, " = ");
            cursor += 2; // cursor needs to be at open_paren when function is called
            result = append(result, create_function_call(tokens[cursor - 1]->string, NULL, "init"));
            
            continue;
         } else if (tokens[cursor]->type == SELF) {
            if (tokens[cursor + 1]->type == DOT) {
                if (tokens[cursor + 2]->type != IDENTIFIER)
                    error("Syntax Error");
                char *temp_identifier = tokens[cursor + 2]->string;
                // two cases: var or func
                if (tokens[cursor + 3]->type == OPEN_PAREN) { // func case 2
                    // gather function params and their types
                    cursor += 3;
                    result = append(result, create_function_call(scope, "self", temp_identifier));
                    continue;
                } else { // variable case 3
                    // static
                    if (array_contains_string(static_vars, static_vars_count, temp_identifier)) {
                        result = append(result, generate_static_name(scope, temp_identifier));
                        result = append(result, " ");
                    } else {
                        result = append(result, "self");
                        result = append(result, "->");
                        result = append(result, temp_identifier);
                        result = append(result, " ");
                    }
                    
                    cursor += 3;
                    continue;
                }
            }
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
                cursor += 3;
                result = append(result, create_function_call(typename, variable_identifier, second_identifier));
                continue;
            } else {
                result = append(result, variable_identifier);
                result = append(result, "->");
                result = append(result, second_identifier);
                cursor += 3;
                continue;
            }  
        } else if (type_name(tokens[cursor]) && tokens[cursor + 1]->type == OPEN_PAREN) {
            cursor += 1;
            result = append(result, create_function_call(tokens[cursor - 1]->string, NULL, "init"));
            continue;
        }
        if (tokens[cursor]->type == CLOSE_CURL_BRACE || tokens[cursor]->type == OPEN_CURL_BRACE) {
            result = append(result, tokens[cursor]->string);
            result = append(result, "\n");
        } else if (tokens[cursor]->type == SEMICOLON) {
            if (result[strlen(result) - 1] == ' ')
                result[strlen(result) - 1] = 0;
            result = append(result, tokens[cursor]->string);
            result = append(result, "\n");
        } else if (tokens[cursor]->type == OPEN_BRACE || tokens[cursor]->type == OPEN_PAREN || tokens[cursor]->type == CLOSE_BRACE || tokens[cursor]->type == CLOSE_PAREN) {
            if (result[strlen(result) - 1] == ' ')
                result[strlen(result) - 1] = 0;
            result = append(result, tokens[cursor]->string);
        } else {
            puts(tokens[cursor]->string);
            result = append(result, tokens[cursor]->string);
            result = append(result, " ");
        }
        cursor++;
    }
    return result;
}

static char * function_definition(char *scope, char **header_string) {
    if (cursor + 4 >= token_count) return NULL; // shortest possible init () {}
    // printf("function definition %s\n", tokens[cursor + 1]->string);
    if (tokens[cursor]->type == FUNC || tokens[cursor]->type == INIT) {
        if (tokens[cursor]->type == FUNC && tokens[cursor + 1]->type != IDENTIFIER) {
            printf("%d\n", tokens[cursor + 1]->type);
            puts(tokens[cursor + 1]->string);
            error("Syntax Error: Expected identifier following 'func' keyword");
        }

        const int e_length = 200;
        char error_message[e_length], 
        *result = "",
        *identifier,
        *arg_string = "(",
        **arg_labels = malloc(MAX_ARG_COUNT),
        *signature = "";
// SCOPE CHANGE
        symbol_top_stack[symbol_top_stack_top++] = symbol_table_top;
        int is_init = 0;

        if (tokens[cursor]->type == INIT) {
            is_init = 1;
            identifier = "init";
            cursor--;
        } else {
            identifier = tokens[cursor + 1]->string;
        }

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
        int arg_count = 0, paren_arg = 0;
        // get arguments
        while (cursor < token_count - 2 && tokens[cursor]->type != CLOSE_PAREN) {
            if (tokens[cursor]->type == IDENTIFIER) {
                char *arg_identifier = tokens[cursor]->string;
                if (tokens[cursor + 1]->type != COLON) {
                    snprintf(error_message, e_length, "Syntax Error: expected colon following identifier. params '%s'", identifier);
                    error(error_message);
                } else if (!type_specifier(tokens[cursor + 2]) && tokens[cursor + 2]->type != OPEN_BRACE) {
                    snprintf(error_message, e_length, "Syntax Error: expected type specifier or array following identifier. params '%s'", identifier);
                    error(error_message);
                }

                if (tokens[cursor + 2]->type == OPEN_BRACE) {
                    paren_arg = 1;
                    cursor++; // skip paren
                }
                char *arg_type = tokens[cursor + 2]->string, *arg = arg_count > 0 ? ", " : "";
                arg = append(arg, arg_type);

                arg = append(arg, " ");

                if (paren_arg) arg = append(arg, "*");

                cursor += 3;
                while(tokens[cursor]->type == OP_MUL) {
                    arg = append(arg, "*");
                    cursor++;
                }
                if (type_name_string(arg_type))
                    arg = append(arg, "*");
                arg = append(arg, arg_identifier);

                if (paren_arg) {
                    cursor++;
                    paren_arg = 0;
                }

                // if (type_name_string(arg_type)) { // have to convert to pointer
                    // arg = malloc(strlen(arg_identifier) + strlen(arg_type) + (arg_count > 0 ? 5 : 3)); // space + , + space + * + 1
                //     sprintf(arg, arg_count > 0 ? ", %s *%s" : "%s *%s", arg_type, arg_identifier);
                // } else {
                    // arg = malloc(strlen(arg_identifier) + strlen(arg_type) + (arg_count > 0 ? 4 : 2)); // space + , + space + 1
                //     sprintf(arg, arg_count > 0 ? ", %s %s" : "%s %s", arg_type, arg_identifier);
                // }
                arg_string = append(arg_string, arg);
                arg_labels[arg_count++] = arg_identifier;
 // symbol table here               
                symbol_table_keys[symbol_table_top] = arg_identifier;
                symbol_table_values[symbol_table_top++] = arg_type;

                continue;
            }
            cursor++;
        }

        if (scope && !is_init) {
            if (arg_count > 0)
                arg_string = append(arg_string, ", ");
            char *temp = malloc(strlen(scope) + 7);
            sprintf(temp, "%s *self", scope);
            arg_string = append(arg_string, temp);
        }
        
        // now two cases, assume void or explicit return type

        if (is_init) {
            if (!scope)
                error("Error: init outside of class declaration");
            result = scope;
            result = append(result, " * ");
            cursor += 2;
        } else if (tokens[cursor + 1]->type == RETURN_SYMBOL) {
            if (!type_specifier(tokens[cursor + 2])) {
                snprintf(error_message, e_length, "Syntax Error: Expected type specifier following return annotation. Function '%s'", identifier);
                error(error_message);
            }
            char *typename = tokens[cursor + 2]->string;
            result = typename;
            result = append(result, " ");
            cursor += 3;
            while(tokens[cursor]->type == OP_MUL) {
                result = append(result, "*");
                cursor++;
            }
            if (type_name_string(typename))
                result = append(result, "* ");
            else
                result = append(result, " ");
            cursor++; // might be wrong
        } else {
            result = "void ";
            cursor += 2;
        }
        if (tokens[cursor - 1]->type != OPEN_CURL_BRACE)
            error("Syntax Error: expected open curly bracket for function body");
        result = append(result, generate_method_signature(scope, identifier, arg_labels, arg_count)); // FIXME need to generate unique name
        result = append(result, arg_string);
        
        signature = append(signature, result);
        signature = append(signature, ");\n");
        *header_string = append(*header_string, signature);
        
        result = append(result, ") {\n");

        // if init, need to add the allocation code
        if (is_init) {
            // we know there's scope because already checked
            char temp[100];
            snprintf(temp, 100, "%s *self = malloc(sizeof(%s));\n", scope, scope);
            result = append(result, temp);
        }

        // if a segmentation fault happens in here, somebody did bad brackets
        result = gather_body(result, scope);
        if (is_init) {
            result[strlen(result) - 2] = 0; // ignore the "}\n"
            result = append(result, "return self;\n");
            result = append(result, "}\n");
        }
// SCOPE CHANGE
        symbol_table_top = symbol_top_stack[--symbol_top_stack_top];

        return result;
    }
    return NULL;
}

static char * class_definition(char **header_string) {
    // puts("class def called");
    if (cursor + 3 >= token_count || tokens[cursor]->type != CLASS) // minimum is class id {}
        return NULL;
    else if (tokens[cursor + 1]->type != IDENTIFIER) 
        error("Syntax Error: expected identifier in class declaration");
    else if (tokens[cursor + 2]->type != OPEN_CURL_BRACE)
        error("Syntax Error: expected open bracket for class declaration");
// SCOPE CHANGE
    symbol_top_stack[symbol_top_stack_top++] = symbol_table_top; // save cursor on stack
    // printf("class definition %s\n", tokens[cursor + 1]->string);
    *header_string = append(*header_string, "\ntypedef struct {\n") ;

    char *method_defs = "",
    *signatures = "", 
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
        // printf("current token: %s\n", tokens[cursor]->string);
        if ((new_item = variable_declaration())) {
            *header_string = append(*header_string, new_item);
        } else if ((new_item = function_definition(identifier, &signatures))) {
            method_defs = append(method_defs, new_item);
        } else if ((new_item = static_variable_definition(identifier))) {
            method_defs = append(method_defs, new_item);
        } else {
            // actually there must be an error...
            puts(identifier);
            puts(tokens[cursor]->string);
            puts(tokens[cursor + 1]->string);
            error("Syntax Error: Expected variable or method definition");
        }
    }
    cursor++; // close curl bracket

// SCOPE CHANGE
    symbol_table_top = symbol_top_stack[--symbol_top_stack_top];
    // printf("cursor: %ld, token_count: %ld\n", cursor, token_count);
    *header_string = append(*header_string, end);
    *header_string = append(*header_string, signatures);
    // struct_def = append(struct_def, method_defs);
    return method_defs;
}

static void first_pass(char **filestring, char **header_string) {
    char *result = "", *temp;
    while (cursor < token_count) {
        if ((temp = class_definition(header_string))) {
            result = append(result, temp);
        } else if ((temp = function_definition(NULL, header_string))) {
            result = append(result, temp);
        } else {
            // look for code to convert
            if (tokens[cursor]->type == OPEN_CURL_BRACE) {
                result = append(result, "{\n");
                cursor++;
                result = gather_body(result, NULL);
            } else {
                result = append(result, tokens[cursor]->string);
            }
            result = append(result, " ");
            cursor++;
        }
    }
    *filestring = append(*filestring, result);
}

void compile(const char *infile, const char *outfile, const char *hfile) {
    FILE *input = fopen(infile, "r"), *output, *header;
    if (!input)
        error("Error: file not found");
    output = fopen(outfile, "w");
    header = fopen(hfile, "w");
    tokens = malloc(sizeof(Token *) * MAX_TOKEN_NUM);
    token_count = tokenize(infile, tokens, MAX_TOKEN_NUM);

    udt_table = malloc(8 * MAX_UDT_COUNT);
    symbol_table_keys = malloc(8 * MAX_SYMBOL_TABLE_COUNT);
    symbol_table_values = malloc(8 * MAX_SYMBOL_TABLE_COUNT);

    static_vars = malloc(MAX_STATIC_VARS);
    printf("token count: %ld\n", token_count);
    for (int i = 0; i < token_count; i++) {
        printf("index: %d, type: %d, value: %s\n", i, tokens[i]->type, tokens[i]->string);
    }
    char *result = "#include \"", *header_string = "#include <stdlib.h>\n";
    result = append(result, (char *) hfile);
    result = append(result, "\"\n");
    first_pass(&result, &header_string);
    fputs(result, output);
    fputs(header_string, header);

}