#include "v2lexer.h"

static Token **tokens;
static size_t tab_spacing = 4, cursor = 0, token_count = 0, current_char = 0, current_line = 1, fs_l = 0, lbi = 0; // literal buffer index
static char *fs, *current_filename, lbuf[LITERAL_BUFFER];
static bool just_got_whitespace = false;
static void error(char *fmt, ...) {
    
    int ret;

    char buffer[500], *message;

    sprintf(buffer, "%s:%lu:%lu: \x1b[31;1merror\x1b[0m : ", current_filename, current_line, current_char);

    /* Declare a va_list type variable */
    va_list myargs;

    /* Initialise the va_list variable with the ... after fmt */

    va_start(myargs, fmt);

    ret = vsprintf(buffer + strlen(buffer), fmt, myargs);

    /* Clean up the va_list */
    va_end(myargs);

    message = malloc(strlen(buffer) + 1);
    strcpy(message, buffer);

    perror(message);

    exit(EXIT_FAILURE);
}

static Token * create_token(TokenType type) {
    Token *result = malloc(sizeof(Token));
    result->type = type;
    result->line_number = current_line;
    result->character = current_char;
    result->filename = current_filename;
    return result;
}

static Token * create_token_with_value(TokenType type, char *value) {
    Token *result = malloc(sizeof(Token));
    result->type = type;
    result->line_number = current_line;
    result->character = current_char;
    result->filename = current_filename;
    result->value = value;
    return result;
}

static Token * create_token_with_value_and_subtype(TokenType type, char *value, char subtype) {
    Token *result = malloc(sizeof(Token));
    result->type = type;
    result->line_number = current_line;
    result->character = current_char;
    result->filename = current_filename;
    result->value = value;
    result->subtype = subtype;
    return result;
}

static inline bool decimal_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool alpha(char c) {
    return c <= 'z' && c >= 'a' || c <= 'Z' && c >= 'A';
}

static inline bool identifier_head(char c) {
    return alpha(c) || c == '_';
}

static inline bool identifier_character(char c) {
    return decimal_digit(c) || identifier_head(c);
}

static inline bool decimal_literal_character(char c) {
    return decimal_digit(c) || c == '_';
}

static inline bool binary_digit(char c) {
    return c == '0' || c == '1';
}

static inline bool binary_literal_character(char c) {
    return c == '0' || c == '1' || c == '_';
}

// this could be wrong since tab probably skips more than 1 character..
static bool inline_space() {
    if (fs[cursor] == ' ') {
        cursor++;
        current_char++;
        return true;
    } else if (fs[cursor] == '\t') {
        cursor++;
        current_char += tab_spacing - (current_char % tab_spacing);
    }
    return false;
}

static bool comment() {
    if (cursor + 1 >= fs_l) return false; // minimum comment length
    if (fs[cursor] == '/' && fs[cursor + 1] == '/') {
        cursor += 2;
        current_char += 2;
        while (cursor < fs_l && fs[cursor] != '\n' && fs[cursor] != '\r') {
            cursor++;
            current_char++;
        }

        if (fs[cursor - 1] == '\n') {
            current_line++;
            current_char = 0;
        }

        if (fs[cursor - 1] == '\r')
            current_char = 0;
        
        if (cursor < fs_l && fs[cursor - 1] == '\r' && fs[cursor] == '\n') {
            cursor++;
            current_line++;
            current_char = 0;
        }
        
        return true;
    }
    return false;
}

static bool multiline_comment() {
    if (cursor + 1 >= fs_l) return false;
    if (fs[cursor] == '/' && fs[cursor + 1] == '*') {
        cursor += 2;
        current_char += 2;
        while (cursor + 1 < fs_l && (fs[cursor] != '*' || fs[cursor + 1] != '/')) {
            current_char++;
            if (fs[cursor] == '\n' || fs[cursor] == '\v')
                current_line++;
            if (fs[cursor] == '\r' || fs[cursor] == '\n')
                current_char = 0;
            cursor++;
        }

        cursor += 2; // skip the close comment
        return true;
    }
    return false;
}

static bool line_break() {
    if (fs[cursor] == '\n') {
        cursor++;
        current_line++;
        current_char = 0;
        return true;
    } else if (fs[cursor] == '\r') {
        current_char = 0; // cr
        if (cursor + 1 < fs_l && fs[cursor + 1] == '\n') {
            cursor += 2;
            current_line++;
            return true;
        }
        cursor++;
        return true;
    }
    return false;
}

// skips whitespace and keeps track of position in file
static bool whitespace_item() {
    if (fs[cursor] == 12)  // form feed
        error("form feed not allowed in source code");
    if (fs[cursor] == 0) { // null.. should i leave this out??
        // throw error
        error("char 'null' not allowed in source code");
    }
    if (fs[cursor] == '\v') { // vertical tab
        cursor++;
        current_line++;
        return true;
    }
    return line_break() 
    || inline_space() 
    || comment()
    || multiline_comment();
}

static bool whitespace() {
    if (whitespace_item()) {
        while(cursor < fs_l && whitespace_item());
        return just_got_whitespace = true;
    }
    return false;
}

// this is probably technically wrong...
static Token * implicit_parameter_name() {
    if (cursor + 1 < fs_l && fs[cursor] == '$') {
        Token *result = create_token(IDENTIFIER);
        do {
            lbuf[lbi++] = fs[cursor++];
        } while(lbi + 1 < DECIMAL_MAX && cursor < fs_l && decimal_digit(fs[cursor]));

        lbuf[lbi] = 0;
        lbi = 0;

        set_value(lbuf, result);
        return result;
    }
    return NULL;
}

static Token * identifier() {
    puts("identifier");
    if (identifier_head(fs[cursor]) || fs[cursor] == '`') {
        Token *result = create_token(IDENTIFIER);

        if (fs[cursor] == '`') {
            if (cursor + 1 >= fs_l) {
                free(result);
                return NULL;
            }
            if (!identifier_head(fs[++cursor]))
                error("expected identifier head following backtick. instead found %c", fs[cursor]);
            
            while (cursor < fs_l && identifier_character(fs[cursor])) {
                if (lbi + 2 >= IDENTIFIER_MAX)
                error("identifier max length exceeded");
                lbuf[lbi++] = fs[cursor++];
                current_char++;
            }

            if (fs[cursor] != '`')
                error("expected backtick to match");
            
            cursor++;

            lbuf[lbi] = 0;
            lbi = 0;

            set_value(lbuf, result);
            return result;
        }

        while (cursor < fs_l && identifier_character(fs[cursor])) {
            if (lbi + 2 >= IDENTIFIER_MAX)
                error("identifier max length exceeded");
            lbuf[lbi++] = fs[cursor++];
            current_char++;
        }

        lbuf[lbi] = 0;
        lbi = 0;

        set_value(lbuf, result);
        return result;
    } else {
        return implicit_parameter_name();
    }
}
// FIXME - leading zeroes should be error
// will skip leading zeroes for decimal literal also

// dont skip leading zeroes if the number IS zero lol
static Token * decimal_literal() {
    puts("decimal literal");
    if (decimal_digit(fs[cursor])) {
        Token *result = create_token(DECIMAL_LITERAL);
        bool skipping_zeroes = true;
        do {
            if (lbi + 2 >= DECIMAL_MAX)
                error("decimal literal length exceeded");
            if (skipping_zeroes && fs[cursor] != '0' && fs[cursor] != '_')
                skipping_zeroes = false;

            if (fs[cursor] == '_' || skipping_zeroes && fs[cursor] == '0') {
                cursor++;
                current_char++;
                continue;
            }

            lbuf[lbi++] = fs[cursor++];
            current_char++;
        } while(cursor < fs_l && decimal_literal_character(fs[cursor]));
        // in case we skipped 0
        if (lbi == 0) 
            lbuf[0] = '0';
        lbuf[lbi] = 0; // null terminate
        lbi = 0; // restore index

        set_value(lbuf, result);
        return result;
    }
    return NULL;
}

// must call this first!
static Token * binary_literal() {
    puts("binary literal");
    // printf("binary literal called with %c%c\n", fs[cursor], fs[cursor + 1]);
    if (cursor + 2 >= fs_l) return false;
    if (fs[cursor] == '0' && fs[cursor + 1] == 'b') {
        Token *result = create_token(BINARY_LITERAL);
        if (!binary_digit(fs[cursor + 2]))
            error("expected binary digit following 0b. instead found %c", fs[cursor + 2]);
        cursor += 2;
// TODO - probably skip leading zeroes later
        bool skipping_zeroes = true;
        while(cursor < fs_l && binary_literal_character(fs[cursor])) {
            if (lbi + 2 >= BINARY_MAX)
                error("binary literal length exceeded");
            if (skipping_zeroes && fs[cursor] == '0') {
                cursor++;
                current_char++;
                continue;
            } else if (skipping_zeroes && fs[cursor] != '_')
                skipping_zeroes = false;
            if (fs[cursor] == '_') {
                cursor++;
                current_char++;
                continue;
            }
            lbuf[lbi++] = fs[cursor++];
            current_char++;
        }
        lbuf[lbi] = 0;
        lbi = 0;

        set_value(lbuf, result);
        return result;
    }
    return NULL;
}
// needs to be changed if I want native bigint support
#define INTEGER_LITERAL_LENGTH 64

// this is hacky
static Token * integer_literal() {
    puts("integer literal");
    Token *t;
    if ((t = binary_literal())) {
        t->type = INTEGER_LITERAL;
        char buf[INTEGER_LITERAL_LENGTH];
        snprintf(buf, INTEGER_LITERAL_LENGTH, "%d", (int) strtol(t->value, NULL, 2));
        t->value = realloc(t->value, strlen(buf) + 1);
        strcpy(t->value, buf);
        return t;
        // conversion logic
    } else if ((t = decimal_literal())) {
        t->type = INTEGER_LITERAL;
        return t;
    }
    return NULL;
} 

static Token * floating_point_literal() {
    return NULL;
}

// static bool escaped_character(char c) {
//     return c == 0 || c == '\\' || c == '\t' || c == '\v' || c == '\f'
//     || c == '\n' || c == '\r' || c == '\'' || c == '\"';
// }

// multiline string will be made with escape char
// or c-style string concatenation
// will be performed by the preprocessor

// interpolated string literal will be handled in the parser later
static Token * string_literal() {
    puts("string literal");
    if (cursor + 1 >= fs_l) return NULL;
    if (fs[cursor] == '"') {
        Token *result = create_token(STATIC_STRING_LITERAL);
        
        cursor++; // skip open quote
        
        while(cursor < fs_l && fs[cursor] != '"') {
            if (cursor + 1 < fs_l && fs[cursor] == '\\' && fs[cursor + 1] == '0') 
                error("null not allowed inside string literal body");
  
            if (lbi + 2 >= LITERAL_BUFFER)
                error("string literal length exceeded");
            // interpolated string
            if (cursor + 1 < fs_l && fs[cursor] == '\\' && fs[cursor + 1] == '(')
                result->type = INTERPOLATED_STRING_LITERAL;
            lbuf[lbi++] = fs[cursor++];
            current_char++;
        }

        if (cursor >= fs_l)
            error("eof reached inside string literal");

        cursor++; // skip close quote

        lbuf[lbi] = 0; // null terminate
        lbi = 0;
        set_value(lbuf, result);
        return result;
    }
    return NULL;
}
// could be shared with identifier
static Token * keyword() {
    // puts("keyword called");
    int type;
    while(lbi < KEYWORD_MAX && cursor + lbi < fs_l && isalpha(fs[cursor + lbi])) {
        lbuf[lbi] = fs[cursor + lbi];
        lbuf[lbi++ + 1] = 0;
        // puts(lbuf);
        // !alpha prevents premature keyword grabbing
        if ((type = is_keyword(lbuf)) != -1 && !identifier_character(fs[cursor + lbi])) {
            Token *result = create_token(type + 1);
            cursor += lbi;
            current_char += lbi;
            lbuf[lbi] = 0;
            lbi = 0;
            set_value(lbuf, result);
            // printf("\n-------------\ngot keyword! here's the token\n");
            // print_token(result);
            return result;
        }
    }
    lbuf[lbi] = 0;
    lbi = 0;
    return NULL;
}

// static inline bool operator_character(char c) {
//     return c == '/' || c == '=' || c == '-' || c == '+'
//     || c == '!' || c == '*' || c == '%' || c == '<'
//     || c == '>' || c == '&' || c == '|' || c == '^'
//     || c == '~' || c == '?';
// }

// allowed operators now
// static Token * operator() {
//     puts("operator");
//     if (operator_character(fs[cursor]) || fs[cursor] == '.') {
//         Token *result = create_token_with_value_and_subtype(OPERATOR, );
//         //check for return annotation
//         if (cursor + 1 < fs_l && fs[cursor] == '-' && fs[cursor + 1] == '>') {
//             result->type = RETURN_ANNOTATION;
//             cursor += 2;
//             return result;
//         }

//         while(lbi < OPERATOR_MAX && cursor < fs_l && (operator_character(fs[cursor]) || fs[cursor] == '.'))
//             lbuf[lbi++] = fs[cursor++];
//         bool ws = just_got_whitespace;
//         if (whitespace()) { // implies invalid to have two operators separated by white space only
//             if (ws) result->fix = INF;
//             else result->fix = POST;
//         } else if (ws)
//             result->fix = PRE;
//     // subtypes
//         if (lbi == 1 && lbuf[0] == '.') result->subtype = '.';
//         else if (lbi == 3 && lbuf[0] == '.' && lbuf[1] == '.' && lbuf[2] == '.') result->subtype = ')'; // variadic
//         else if (lbuf[lbi - 1] == '.') result->fix = POST;
//         else if (lbi == 1 && operator_character(lbuf[0])) {
//             printf("subtype: %c\n", lbuf[0]);
//             result->subtype = lbuf[0];
//         }
        

//         lbuf[lbi] = 0;
//         lbi = 0;

//         set_value(lbuf, result);
//         return result;
//     }
//     return NULL;
// }

/* operators are the following

%, &, -, +, /, !, ?, *, ^, |, ||, &&, ++, --, 
*=, %=, /=, +=, -=, <, >, <=, >=, ==, ==, ^=, !=, ??

*/
static Token *operator() {
    Token *t;
    switch (fs[cursor]) {
        case '+':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '+' && cursor++)
                t = create_token_with_value(OPERATOR, "++");
            else if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "+=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "+", '+');
            break;
        case '-':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '-' && cursor++)
                t = create_token_with_value(OPERATOR, "--");
            else if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "+=");
            else if (cursor + 1 < fs_l && fs[cursor + 1] == '>' && cursor++)
                t = create_token_with_value(RETURN_ANNOTATION, "->");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "-", '-');
            break;
        case '*':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "*=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "*", '*');
            break;
        case '/':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "/=");
            else
                 t = create_token_with_value_and_subtype(OPERATOR, "/", '/');
            break;
        case '%':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "%=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "%", '%');
            break;
        case '^':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "^=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "^", '^');
            break;
        case '!':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "!=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "!", '!');
            break;
        case '&':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '&' && cursor++)
                t = create_token_with_value(OPERATOR, "&&");
            else if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "&=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "&", '&');
            break;
        case '|':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '|' && cursor++)
                t = create_token_with_value(OPERATOR, "||");
            else if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "|=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "|", '|');
            break;
        case '<':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "<=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "<", '<');
            break;
        case '>':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, ">=");
            else
                t = create_token_with_value_and_subtype(OPERATOR, ">", '>');
            break;
        case '=':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '=' && cursor++)
                t = create_token_with_value(OPERATOR, "==");
            else
                t = create_token_with_value_and_subtype(OPERATOR, "=", '=');
            break;
        case '?':
            if (cursor + 1 < fs_l && fs[cursor + 1] == '?' && cursor++)
                t = create_token_with_value_and_subtype(OPERATOR, "??", 'N'); // nil coalesce operator
            else
                t = create_token_with_value_and_subtype(OPERATOR, "?", '?');
            break;
        default:
            return NULL;
    }
    cursor++;
    bool ws = just_got_whitespace;
    if (whitespace()) { // implies invalid to have two operators separated by white space only
        if (ws) t->fix = INF;
        else t->fix = POST;
    } else if (ws)
        t->fix = PRE;
    else
        t->fix = INF;
    return t;
}

// make sure I add logic for import statements and include statements
void lex(char *filestring, size_t length, Token **tks, size_t *tkl, char *filename) {
    tokens = tks;
    fs = filestring;
    fs_l = length;
    current_filename = filename;
    Token *current_token = NULL;
    while (cursor < fs_l) {
        // printf("current character: %c\n", fs[cursor]);
        if (whitespace()) continue;
        else if ((current_token = integer_literal()) // must be called before operator for '-'
        || (current_token = floating_point_literal())
        || (current_token = string_literal())
        || (current_token = operator())
        || (current_token = keyword()) // must be before identifier
        || (current_token = identifier())) {
            tokens[token_count++] = current_token;
            // printf("got token at line %lu ", current_line);
            // print_token(current_token);
            just_got_whitespace = false;
            continue;
        }
        switch(fs[cursor]) {
            case '(':
                tokens[token_count++] = create_token_with_value(OPEN_PAREN, "(");
                break;
            case ')':
                tokens[token_count++] = create_token_with_value(CLOSE_PAREN, ")");
                break;
            case '{':
                tokens[token_count++] = create_token_with_value(OPEN_CURL_BRACE, "{");
                break;
            case '}':
                tokens[token_count++] = create_token_with_value(CLOSE_CURL_BRACE, "}");
                break;
            case '[':
                tokens[token_count++] = create_token_with_value(OPEN_SQ_BRACE, "]");
                break;
            case ']':
                tokens[token_count++] = create_token_with_value(CLOSE_SQ_BRACE, "]");
                break;
            case ',':
                tokens[token_count++] = create_token_with_value(COMMA, ",");
                break;
            case ':':
                tokens[token_count++] = create_token_with_value(COLON, ":");
                break;
            case ';':
                tokens[token_count++] = create_token_with_value(SEMICOLON, ";");
                break;
            case '#':
                tokens[token_count++] = create_token_with_value(HASHTAG, "#");
                break;
            case '_':
                tokens[token_count++] = create_token_with_value(WILDCARD, "_");
                break;
            case '.':
                tokens[token_count++] = create_token_with_value(DOT, ".");
                break;
            default:
                error("unrecognized character in file %c", fs[cursor]);
        }
        just_got_whitespace = false;
        cursor++;
    }

    *tkl = token_count;
}