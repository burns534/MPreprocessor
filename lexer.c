#include "lexer.h"

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
static const char *keywords[] = {
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
    "break",
    "continue",
    "return",
    "float",
    "int",
    "double",
    "long",
    "char",
    "void",
    "class",
    "super",
    "self",
    "init"
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
    for(int i = 0; i < TOKEN_MAX_LENGTH - 1; i++) {
        token_string[i] = current_char;
        if ((current_char = fgetc(file)) == '"') {
            token_string[i + 1] = 0;
            break;
        }
    }
    return create_token(STRING_LITERAL, token_string);
}

static Token * get_number_token(FILE *file, char first_char) {
    char *number_string = malloc(TOKEN_MAX_LENGTH);
    fpos_t current_position;
    for (int i = 0; i < TOKEN_MAX_LENGTH; i++) {
        number_string[i] = first_char;
        if (!isdigit((first_char = fgetc(file))))
            break;
    }
    // should fix the ending
    backtrack(file, 1);
    
    return create_token(CONSTANT, number_string);
}

static Token * get_string_token(FILE *file, char first_char) {
    char *token_string = malloc(TOKEN_MAX_LENGTH);
    int index;
    for(int i = 0; i < TOKEN_MAX_LENGTH; i++) {
        token_string[i] = first_char;
        if ((index = is_keyword(token_string)) != -1)
            break;
        first_char = fgetc(file);
        // gotta backtrack if this happens
        if (!isalnum(first_char) && first_char != '_') {
            backtrack(file, 1);
            break;
        }
    }
    
    index = index == -1 ? IDENTIFIER : index;
    return create_token(index, token_string); // this is int to TokenType implicit casting based on the array ordering
}

static void error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

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
                    tokens[token_count++] = create_token(OP_ADD, "+");
                    break;
                case '-':
                    tokens[token_count++] = create_token(OP_SUB, "-");
                    break;
                case '*':
                    tokens[token_count++] = create_token(OP_MUL, "*");
                    break;
                case '/':
                    if ((current_char = fgetc(file)) == '/')
                        tokens[token_count++] = create_token(COMMENT, "//");
                    else {
                        tokens[token_count++] = create_token(OP_DIV, "/");
                        backtrack(file, 1);
                    }
                    break;
                case '<':
                    tokens[token_count++] = create_token(OP_LT, "<");
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
                default: // is keyword or other
                    break;
            }
        }
    }
    return 0;
}