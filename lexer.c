#include "lexer.h"

// #define DEBUG_LEX 1

static int isalpha(char c) {
    // printf("isalpha: %d\n", c);
    return c < 123 && c > 96 || c > 64 && c < 91;
}

static int isdigit(char c) {
    return c < 58 && c > 47;
}

static int isalnum(char c) {
    return isdigit(c) || isalpha(c);
}
// order must agree with order of TokenType enum
static const char * keywords[NUM_KEYWORDS] = {
    "if",
    "else",
    "switch",
    "case",
    "default",
    "func",
    "var",
    "let",
    "enum",
    "while",
    "sizeof",
    "true",
    "false",
    "break",
    "continue",
    "return",
    "float",
    "int",
    "double",
    "long",
    "char",
    "void",
    "ulong",
    "bool",
    "nil",
    "class",
    "super",
    "self",
    "init",
    "static"
};

static void backtrack(FILE *file, int num_chars) {
    fpos_t current_pos;
    fgetpos(file, &current_pos);
    current_pos -= num_chars;
    fsetpos(file, &current_pos);
}

static int is_keyword(char *string) {
    for (int i = 0; i < NUM_KEYWORDS; i++) {
        if (strcmp(string, keywords[i]) == 0)
            return i;
    }
    return -1;
}

// cursor should point to last quotation mark after completion
// all of these helper functions should follow this pattern

// also can't have a string literal longer than 255 bytes
static Token * get_string_literal(FILE *file) {
    char current_char, *token_string = malloc(TOKEN_MAX_LENGTH);
    token_string[0] = '"';
    int i = 1;
    for(; i < TOKEN_MAX_LENGTH - 1; i++) {
        current_char = fgetc(file);
        if (current_char == '"') {
            token_string[i] = current_char;
            break;
        }
        token_string[i] = current_char;
    }
    token_string[i + 1] = 0;
    return create_token(STRING_LITERAL, token_string);
}

static Token * get_number_token(FILE *file, char first_char) {
    char *number_string = malloc(TOKEN_MAX_LENGTH);
    fpos_t current_position;
    for (int i = 0; i < TOKEN_MAX_LENGTH; i++) {
        number_string[i] = first_char;
        first_char = fgetc(file);
        if (!isdigit(first_char) && first_char != '.')
            break;
    }
    // should fix the ending
    backtrack(file, 1);
    
    return create_token(CONSTANT, number_string);
}

static Token * get_string_token(FILE *file, char first_char) {
    char *token_string = malloc(TOKEN_MAX_LENGTH);
    int index;
    int matched_keyword = 0;
    for(int i = 0; i < TOKEN_MAX_LENGTH; i++) {
        matched_keyword = 0;
        token_string[i] = first_char;
        if ((index = is_keyword(token_string)) != -1) 
            matched_keyword = 1;
        first_char = fgetc(file);
        // gotta backtrack if this happens
        if (!isalnum(first_char) && first_char != '_') {
            backtrack(file, 1);
            break;
        }
    }
    if (matched_keyword && strcmp(keywords[index], "ulong") == 0)
        token_string = "unsigned long";
    else if (matched_keyword && strcmp(keywords[index], "bool") == 0)
        token_string = "unsigned char";
    else if (matched_keyword && strcmp(keywords[index], "true") == 0)
        token_string = "1";
    else if (matched_keyword && strcmp(keywords[index], "false") == 0)
        token_string = "0";
    else if (matched_keyword && strcmp(keywords[index], "nil") == 0)
        token_string = "NULL";
    return create_token(matched_keyword ? index: IDENTIFIER, token_string);
}

static void error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// return number of tokens
int tokenize(const char *filename, Token **tokens, int tokens_length) {
    int cursor = 0, token_count = 0;
    char current_char, token_string[TOKEN_MAX_LENGTH];
    FILE *file = fopen(filename, "r");
    while(token_count < tokens_length) {
        if ((current_char = fgetc(file)) == EOF)
            break;
#if DEBUG_LEX
        printf("current_char: %c\n", current_char);
#endif
        // must be number
        if (isdigit(current_char))
            tokens[token_count++] = get_number_token(file, current_char);
        else if (isalpha(current_char) || current_char == '_')
            tokens[token_count++] = get_string_token(file, current_char);
        else {
            switch(current_char) {
                case '#': {
                    // get include
                    char *buffer = malloc(100), buffer_index = 10, condition = '"';
                    // skip whitespace
                    while (current_char != '"' && current_char != '<') 
                        current_char = fgetc(file);
                    snprintf(buffer, 100, "#include %c", current_char);

                    if (current_char == '<')
                        condition = '>';
                    while ((current_char = fgetc(file)) != condition)
                        buffer[buffer_index++] = current_char;
                    buffer[buffer_index] = current_char;
                    buffer[buffer_index + 1] = '\n';
                    buffer[buffer_index + 2] = 0;
                    tokens[token_count++] = create_token(INCLUDE_STATMENT, buffer);
                } break;
                case '!':
                    if ((current_char = fgetc(file)) == '=')
                        tokens[token_count++] = create_token(OP_NE, "!=");
                    else {
                        tokens[token_count++] = create_token(OP_NOT, "!");
                        backtrack(file, 1);
                    }
                    break;
                case ' ':
                    break;
                case '(':
                    tokens[token_count++] = create_token(OPEN_PAREN, "(");
                    break;
                case ')':
                    tokens[token_count++] = create_token(CLOSE_PAREN, ")");
                    break;
                case '[':
                    tokens[token_count++] = create_token(OPEN_BRACE, "[");
                    break;
                case ']':
                    tokens[token_count++] = create_token(CLOSE_BRACE, "]");
                    break;
                case '{':
                    tokens[token_count++] = create_token(OPEN_CURL_BRACE, "{");
                    break;
                case '}':
                    tokens[token_count++] = create_token(CLOSE_CURL_BRACE, "}");
                    break;
                case '+':
                    if ((current_char = fgetc(file)) == '=')
                        tokens[token_count++] = create_token(OP_ADD_EQ, "+=");
                    else if (current_char == '+')
                        tokens[token_count++] = create_token(OP_INC, "++");
                    else {
                        tokens[token_count++] = create_token(OP_ADD, "+");
                        backtrack(file, 1);
                    }
                    break;
                case '-':
                    if ((current_char = fgetc(file)) == '>')
                        tokens[token_count++] = create_token(RETURN_SYMBOL, "->");
                    else if (current_char == '=')
                        tokens[token_count++] = create_token(OP_SUB_EQ, "-=");
                    else if (current_char == '-')
                        tokens[token_count++] = create_token(OP_DEC, "--");
                    else {
                        tokens[token_count++] = create_token(OP_SUB, "-");
                        backtrack(file, 1);
                    }
                    break;
                case '*':
                    if ((current_char = fgetc(file)) == '=')
                        tokens[token_count++] = create_token(OP_MUL_EQ, "*=");
                    else {
                        tokens[token_count++] = create_token(OP_MUL, "*");
                        backtrack(file, 1);
                    }
                    break;
                case '/':
                    if ((current_char = fgetc(file)) == '/')
                        // skip comment
                        
                        while ((current_char = fgetc(file)) != '\n');
                    else if (current_char == '*') {
                        // multiline comment
                        while (current_char != EOF) {
                            current_char = fgetc(file);
                            if (current_char == '*') {
                                if ((current_char = fgetc(file)) == '/')
                                    break;
                                else
                                    backtrack(file, 1);
                            }
                        }
                    } else if (current_char == '=')
                        tokens[token_count++] = create_token(OP_DIV_EQ, "/=");
                    else {
                        tokens[token_count++] = create_token(OP_DIV, "/");
                        backtrack(file, 1);
                    }
                    break;
                case '<':
                    if ((current_char = fgetc(file)) == '=') {
                        tokens[token_count++] = create_token(OP_LE, "<=");
                    } else {
                        tokens[token_count++] = create_token(OP_LT, "<");
                        backtrack(file, 1);
                    }
                    break;
                case '>':
                    if ((current_char = fgetc(file)) == '=') {
                        tokens[token_count++] = create_token(OP_GE, ">=");
                    } else {
                        tokens[token_count++] = create_token(OP_GT, ">");
                        backtrack(file, 1);
                    }
                    break;
                case '%':
                    tokens[token_count++] = create_token(OP_MOD, "%");
                    break;
                case ';':
                    tokens[token_count++] = create_token(SEMICOLON, ";");
                    break;
                case ':':
                    tokens[token_count++] = create_token(COLON, ":");
                    break;
                case ',':
                    tokens[token_count++] = create_token(COMMA, ",");
                    break;
                case '.':
                    tokens[token_count++] = create_token(DOT, ".");
                    break;
                case '=':
                    if ((current_char = fgetc(file)) == '=') {
                        tokens[token_count++] = create_token(OP_EQ, "==");
                    } else {
                        tokens[token_count++] = create_token(EQ, "=");
                        backtrack(file, 1);
                    }
                    break;
                case '"':
                    tokens[token_count++] = get_string_literal(file);
                    break;
                case '&':
                    if ((current_char = fgetc(file)) == '&')
                        tokens[token_count++] = create_token(OP_AND, "&&");
                    else
                        backtrack(file, 2);
                    break;
                case '|':
                    if ((current_char = fgetc(file)) == '|')
                        tokens[token_count++] = create_token(OP_OR, "||");
                    else
                        backtrack(file, 2);
                    break;
                default: // is keyword or other
                    break;
            }
        }
        // printf("got token: %s\n", tokens[token_count - 1]->string);
    }
    return token_count;
}