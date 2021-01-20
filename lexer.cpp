//
//  Lexer.c
//  MCompiler
//
//  Created by Kyle Burns on 1/6/21.
//
// need to have a token deinit that frees the strings stored
// look into bitwise NOT ~ later
// FIXME: LOOK FOR SIZEOF
// investigate typename issue also
// currently an issue with lexer returning incorrect number of tokens and or not catching the closing bracket...
#include "lexer.h"

// 41 items 32 standard + 9 custom keywords
static const char * keywords[] = {"sizeof", "typedef", "extern", "static", "auto", "register", "char", "short", "int", "long", "signed", "unsigned", "float", "double", "bool", "const", "volatile", "void", "override", "private", "self", "super", "init", "deinit", "struct", "union", "enum", "class", "protocol", "case", "default", "if", "else", "switch", "while", "do", "for", "goto", "continue", "break", "return"};

static TokenType tokenTypes[] = {SIZEOF, TYPEDEF, EXTERN, STATIC, AUTO, REGISTER, CHAR, SHORT, INT, LONG, SIGNED, UNSIGNED, FLOAT, DOUBLE, BOOL, CONST, VOLATILE, VOID, OVERRIDE, PRIVATE, SELF, SUPER, INIT, DEINIT, STRUCT, UNION, ENUM, CLASS, PROTOCOL, CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO, CONTINUE, BREAK, RETURN};

const char * stringRepresentation(MToken * token) {
    assert(token != NULL);
//    printf("value/type: %d/%s\n", token->type, token->value);
    if (token->type == IDENTIFIER) {
        return "Identifier";
    } else if (token->type == CONSTANT) {
        return "Constant";
    } else if (token->type == STRING_LITERAL) {
        return "string literal";
    }
    for(int i = 0; i < NUM_TOKENS; i++)
        if (token->type == tokenTypes[i])
            return keywords[i];
    return token->value;
}

// this is going to allow some seriously error laden code to make it through
// returns index of key or -1 if not present
int isKeyword(char * string) {
    for(int i = 0; i < NUM_TOKENS; i++)
        if (strcmp(keywords[i], string) == 0)
            return i;
    return -1;
}

TokenType getTokenType(int index) {
    assert(index < NUM_TOKENS);
    return tokenTypes[index];
}

static void error(const char * message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// this won't catch number terminated by decimal as it currently stands, e.g. "2." instead of "2.0"
// returns
static MToken * numToken(FILE *fp, char *c) {
    char *numBuffer = (char *) malloc(MAX_NUMBER_LENGTH);
    for (size_t i = 0; !feof(fp) && i < MAX_NUMBER_LENGTH; i++) {
        printf("numtoken with c/buffer: %c/%s\n", *c, numBuffer);
        if (!isdigit(*c) && *c != '.') return new MToken(CONSTANT, numBuffer);
        numBuffer[i] = *c;
        *c = fgetc(fp);
    }
    error("Error: Syntax Error: numToken");
    return NULL;
}
// fp needs to be advanced after this function
static MToken * stringToken(FILE *fp, char *c, int * macroMode) {
    char *stringBuffer = (char *) malloc(MAX_IDENTIFIER_LENGTH);
    for (size_t i = 0; !feof(fp) && i < MAX_IDENTIFIER_LENGTH; i++) {
        // printf("stringToken with c/string: %c/%s\n", *c, stringBuffer);
        if (!isalnum(*c) && *c != '_') {
            int index = isKeyword(stringBuffer);
            if (index >= 0) {
                *macroMode = 0;
                return new MToken(getTokenType(index), stringBuffer);
            } else {
                stringBuffer[i] = 0;
                return new MToken(IDENTIFIER, stringBuffer);
            }
        }
        stringBuffer[i] = *c;
        *c = fgetc(fp);
    }
    error("Syntax Error: stringToken");
    return NULL;
}

static MToken * stringLiteral(FILE *fp, char *c) {
    char *buffer = (char *) malloc(DEFAULT_STRING_BUFFER_LENGTH);
    size_t bufferSize = DEFAULT_STRING_BUFFER_LENGTH;
    *c = fgetc(fp);
    size_t i = 0;
    for(; !feof(fp) && *c != 34; i++) {
        // printf("string literal with c/string: %c/%s\n", *c, buffer);
        if (i > 0.6 * bufferSize) {
            bufferSize *= 4;
            buffer = (char *) realloc(buffer, 8 * bufferSize);
        }
        if(*c == 92) {
            buffer[i++] = *c;
            *c = fgetc(fp);
            assert(*c != -1);
        }
        buffer[i] = *c;
        *c = fgetc(fp);
    }
    buffer[i + 1] = 0;
    return new MToken(STRING_LITERAL, buffer);
}

static void comment(FILE *fp, int lineComment) {
    char c;
    while(1) {
        c = fgetc(fp);
    start:
        // printf("comment c: %c\n", c);
        if (lineComment) {
            if (c == '\n') break;
        } else {
            if (c == '*') {
                c = fgetc(fp);
                if (c == '/') break;
                goto start;
            }
        }
    }
    return;
}

MToken ** lex(FILE *fp, size_t *count) {
    size_t bufferSize = DEFAULT_TOKEN_BUFFER_SIZE;
    *count = -1;
    MToken ** buffer = (MToken **) malloc(bufferSize * 8);
    char c = fgetc(fp);
    int macroMode = 1;
    // string token i.e. type definition must be followed by space. c will hold that space at the end of this loop
    while(!feof(fp) && macroMode) {
        // printf("macromode c: %c\n", c);
        if (c == '/') {
            c = fgetc(fp);
            if (c == '*') comment(fp, 0);
            else if (c == '/') comment(fp, 1);
            else error("Erroneous '/'");
        }
        if (isalnum(c)) {
            MToken * result = stringToken(fp, &c, &macroMode);
            if (!macroMode) {
                buffer[++*count] = result;
                printf("got string token with count: %s/%lu\n", result->value, *count);
                break; // note this
            }
        }
        c = fgetc(fp);
    }
loop:
    while(!feof(fp)) {
        printf("main loop with c/count: %c/%lu\n", c, *count);
        if (*count > 0.6 * bufferSize) {
            bufferSize *= 4;
            buffer = (MToken **) realloc(buffer, 8 * bufferSize);
        }
        // process digit
        if (isdigit(c)) {
            buffer[++*count] = numToken(fp, &c);
            printf("got num token %f with count %lu\n", atof(buffer[*count]->value), *count);
            goto loop;
        }
        // process string
        else if (isalpha(c) || c == '_') {
            buffer[++*count] = stringToken(fp, &c, &macroMode);
            printf("got string token with count: %s/%lu\n", buffer[*count]->value, *count);
            // c = fgetc(fp);
            goto loop;
        }
        // symbols
        switch (c) {
            case ';': buffer[++*count] = new MToken((int)';', ";"); break;
            case '{': buffer[++*count] = new MToken((int)'{', "{"); break;
            case '}': buffer[++*count] = new MToken((int)'}', "}"); break;
            case '[': buffer[++*count] = new MToken((int)'[', "["); break;
            case ']': buffer[++*count] = new MToken((int)']', "]"); break;
            case ',': buffer[++*count] = new MToken((int)',', ","); break;
            case ':': buffer[++*count] = new MToken((int)':', ":"); break;
            case '?': buffer[++*count] = new MToken((int)'?', "?"); break;
            case '(': buffer[++*count] = new MToken((int)'(', "("); break;
            case ')': buffer[++*count] = new MToken((int)')', ")"); break;
            case '~': buffer[++*count] = new MToken((int)'~', "~"); break;
            case 34:
                buffer[++*count] = stringLiteral(fp, &c);
                printf("got string literal with count: %s/%lu\n", buffer[*count]->value, *count);
                break;
            case '=':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(EQ_OP, "==");
                } else {
                    buffer[++*count] = new MToken((int)'=', "=");
                    goto loop;
                }
                break;
            case '.':
                c = fgetc(fp);
                if (c != '.') {
                    buffer[++*count] = new MToken((int)'.', ".");
                    goto loop;
                } else {
                    if (fgetc(fp) != '.') error("Syntax Error");
                    buffer[++*count] = new MToken(ELLIPSES, "...");
                }
                break;
            case '^':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(XOR_ASSIGN, "^=");
                } else {
                    buffer[++*count] = new MToken((int)'^', "^");
                    goto loop;
                }
                break;
            case '!':
                c = fgetc(fp);
                if (c == '!') {
                    buffer[++*count] = new MToken(NE_OP, "!=");
                } else {
                    buffer[++*count] = new MToken((int)'!', "!");
                    goto loop;
                }
                break;
            case '+':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(ADD_ASSIGN, "+=");
                } else if (c == '+') {
                    buffer[++*count] = new MToken(INC_OP, "++");
                } else {
                    buffer[++*count] = new MToken((int)'+', "+");
                    goto loop;
                }
                break;
            case '-':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(SUB_ASSIGN, "-=");
                } else if (c == '-') {
                    buffer[++*count] = new MToken(DEC_OP, "--");
                } else if (c == '>') {
                    buffer[++*count] = new MToken(PTR_OP, "->");
                } else {
                    buffer[++*count] = new MToken((int)'-', "-");
                    goto loop;
                }
                break;
            case '*':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(MUL_ASSIGN, "*=");
                } else {
                    buffer[++*count] = new MToken((int)'*', "*");
                    goto loop;
                }
                break;
            case '/':
                c = fgetc(fp);
                if (c == '/') {
                    comment(fp, 1);
                } else if (c == '*') {
                    comment(fp, 0);
                } else if (c == '=') {
                    buffer[++*count] = new MToken(DIV_ASSIGN, "/=");
                } else {
                    buffer[++*count] = new MToken((int)'/', "/");
                    goto loop;
                }
                break;
            case '%':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(MOD_ASSIGN, "%=");
                } else {
                    buffer[++*count] = new MToken((int)'%', "%");
                    goto loop;
                }
                break;
            case '|':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(OR_ASSIGN, "|=");
                } else if (c == '|') {
                    buffer[++*count] = new MToken(OR_OP, "||");
                } else {
                    buffer[++*count] = new MToken((int)'|', "|");
                    goto loop;
                }
                break;
            case '&':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(AND_ASSIGN, "&=");
                } else if (c == '&') {
                    buffer[++*count] = new MToken(AND_OP, "&&");
                } else {
                    buffer[++*count] = new MToken((int)'&', "&");
                    goto loop;
                }
                break;
            case '>':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(GE_OP, ">=");
                } else if (c == '>') {
                    c = fgetc(fp);
                    if (c == '=') buffer[++*count] = new MToken(RIGHT_ASSIGN, ">>=");
                    else {
                        buffer[++*count] = new MToken(RIGHT_OP, ">>");
                        goto loop;
                    }
                } else {
                    buffer[++*count] = new MToken((int)'>', ">");
                    goto loop;
                }
                break;
            case '<':
                c = fgetc(fp);
                if (c == '=') {
                    buffer[++*count] = new MToken(LE_OP, "<=");
                } else if (c == '<') {
                    c = fgetc(fp);
                        if (c == '=') buffer[++*count] = new MToken(LEFT_ASSIGN, "<<=");
                    else {
                        buffer[++*count] = new MToken(LEFT_OP, "<<");
                        goto loop;
                    }
                } else {
                    buffer[++*count] = new MToken((int)'<', "<");
                    goto loop;
                }
                break;
            default: break;
        }
        c = fgetc(fp); // advance to next char
    }
    printf("at the end with count: %lu\n", *count);
    ++*count;
    return buffer;
}
 
 
