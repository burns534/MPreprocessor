#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H
#pragma once
#include <stdbool.h>
#include <string.h>
// has to be in specific order for indexing to work
// does not have bitwise ops
typedef enum {
    NONE,
    // used in statements
    BREAK,
    CASE,
    CATCH,
    CONTINUE,
    DEFAULT,
    DO,
    ELSE,
    FALLTHROUGH,
    FOR,
    GUARD,
    IF,
    IN,
    REPEAT,
    RETURN,
    THROW,
    SWITCH,
    WHERE,
    WHILE,

    // used in declarations
    CLASS,
    DEINIT,
    ENUM,
    EXTENSION,
    FUNC,
    IMPORT, // probably don't need this
    INIT,
    LET,
    OPERATOR,
    PRIVATE,
    PROTOCOL,
    PUBLIC,
    RETHROWS,
    STATIC,
    STRUCT,
    SUBSCRIPT,
    TYPEDEF,
    VAR,

    // used in expressions and types
    ANY,
    AS,
    FALSE,
    IS,
    NIL,
    SELF,
    SUPER,
    THROWS,
    TRUE,
    TRY,

    // special keywords
    CONVENIENCE,
    DIDSET,
    FINAL,
    GET,
    LAZY,
    OVERRIDE,
    REQUIRED,
    SET,
    WEAK,
    WILLSET,

    // used in patterns
    WILDCARD,

    // keywords beginning in hash symbol
    SELECTOR,

    // types
    FLOAT,
    INT,
    DOUBLE,             // double
    LONG,               // long
    CHAR,               // char
    VOID,               // void
    ULONG,              // ulong
    UINT,
    BOOL,               // bool

    // punctutation
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_CURL_BRACE,
    CLOSE_CURL_BRACE,
    OPEN_SQ_BRACE,
    CLOSE_SQ_BRACE,
    DOT, // already an operator
    COMMA,
    COLON,
    SEMICOLON,
    EQ, // already operator
    HASHTAG,
    AND,            // as prefix ??
    RETURN_ANNOTATION,
    BACKTICK, // only relevant in identifier
    QUESTION_MARK, // handled in operator
    EXCLAMATION_MARK, // as postfix ??

    // literals
    DECIMAL_LITERAL,
    BINARY_LITERAL,
    STATIC_STRING_LITERAL,
    INTERPOLATED_STRING_LITERAL,

    IDENTIFIER,

    // other
    INCLUDE_STATEMENT

} TokenType;

static char * lookup_table[] = {
    "NONE", 
    "BREAK",
    "CASE",
    "CATCH",
    "CONTINUE",
    "DEFAULT",
    "DO",
    "ELSE",
    "FALLTHROUGH",
    "FOR",
    "GUARD",
    "IF",
    "IN",
    "REPEAT",
    "RETURN",
    "THROW",
    "SWITCH",
    "WHERE",
    "WHILE",

    "CLASS",
    "DEINIT",
    "ENUM",
    "EXTENSION",
    "FUNC",
    "IMPORT",
    "INIT",
    "LET",
    "OPERATOR",
    "PRIVATE",
    "PROTOCOL",
    "PUBLIC",
    "RETHROWS",
    "STATIC",
    "STRUCT",
    "SUBSCRIPT",
    "TYPEDEF",
    "VAR",

    "ANY",
    "AS",
    "FALSE",
    "IS",
    "NIL",
    "SELF",
    "SUPER",
    "THROWS",
    "TRUE",
    "TRY",

    "CONVENIENCE",
    "DIDSET",
    "FINAL",
    "GET",
    "LAZY",
    "OVERRIDE",
    "REQUIRED",
    "SET",
    "WEAK",
    "WILLSET",

    "WILDCARD",

    "SELECTOR",

    "FLOAT",
    "INT",
    "DOUBLE",
    "LONG",
    "CHAR",
    "VOID",
    "ULONG",
    "UINT",
    "BOOL",

    "OPEN_PAREN",
    "CLOSE_PAREN",
    "OPEN_CURL_BRACE",
    "CLOSE_CURL_BRACE",
    "OPEN_SQ_BRACE",
    "CLOSE_SQ_BRACE",
    "DOT",
    "COMMA",
    "COLON",
    "SEMICOLON",
    "EQ",
    "HASHTAG",
    "AND",
    "RETURN_ANNOTATION",
    "BACKTICK",
    "QUESTION_MARK",
    "EXCLAMATION_MARK",

    "DECIMAL_LITERAL",
    "BINARY_LITERAL",
    "STATIC_STRING_LITERAL",
    "INTERPOLATED_STRING_LITERAL",

    "IDENTIFIER",

    "INCLUDE STATEMENT"
};

static char *keyword_table[] = {
    "break",
    "case",
    "catch",
    "continue",
    "default",
    "do",
    "else",
    "fallthrough",
    "for",
    "guard",
    "if",
    "in",
    "repeat",
    "return",
    "throw",
    "switch",
    "where",
    "while",

    "class",
    "deinit",
    "enum",
    "extension",
    "func",
    "import",
    "init",
    "let",
    "operator",
    "private",
    "protocol",
    "public",
    "rethrows",
    "static",
    "struct",
    "subscript",
    "typedef",
    "var",

    "Any",
    "as",
    "false",
    "is",
    "nil",
    "self",
    "super",
    "throws",
    "true",
    "try",

    "convenience",
    "didset",
    "final",
    "get",
    "lazy",
    "override",
    "required",
    "set",
    "weak",
    "willset"
};

char * token_type_to_string(TokenType t) {
    return lookup_table[t];
}

char * keyword_for_type(TokenType t) {
    if (t > 55 || t < 1) {
        perror("invalid type");
        exit(1);
    }
    return keyword_table[t - 1];
}

int is_keyword(char *s) {
    for (int i = 0; i < 56; i++) {
        if (strcmp(s, keyword_table[i]) == 0)
            return i;
    }
    return -1;
}

#endif