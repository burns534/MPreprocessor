#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H
#pragma once
#define NUM_KEYWORDS 29
// has to be in specific order for indexing to work
// does not have bitwise ops
typedef enum {
    IF, 
    ELSE,
    SWITCH,
    CASE,
    DEFAULT,
    FUNC, 
    VAR, 
    LET,  
    ENUM, 
    WHILE,
    SIZEOF,
    TRUE,
    FALSE,
    // flow control
    BREAK,
    CONTINUE,
    RETURN,
    // primitives
    FLOAT,
    INT,
    DOUBLE,             // double
    LONG,               // long
    CHAR,               // char
    VOID,               // void
    ULONG,           // ulong
    BOOL,

    // class
    CLASS,              // class
    SUPER,              // super
    SELF,               // self
    INIT,               // init
    STATIC,             // static

    OP_ADD,             // +
    OP_SUB,             // -
    OP_MUL,             // *
    OP_DIV,             // /
    OP_AND,             // &&
    OP_OR,              // ||
    OP_LT,              // <
    OP_GT,              // >
    OP_GE,              // >=
    OP_LE,              // <=
    OP_EQ,              // ==
    OP_MOD,             // %
    OP_MUL_EQ,          // *=
    OP_DIV_EQ,          // /=
    OP_ADD_EQ,          // +=
    OP_SUB_EQ,          // -=
    OP_INC,             // ++
    OP_DEC,             // --
    OPEN_BRACE,         // [
    CLOSE_BRACE,        // ]
    OPEN_CURL_BRACE,    // {
    CLOSE_CURL_BRACE,   // }
    OPEN_PAREN,         // ()
    CLOSE_PAREN,        // )
    SEMICOLON,          // ;
    COLON,              // :
    COMMA,              // ,
    DOT,                // .
    EQ,                 // =
    STRING_LITERAL,     // ""
    CONSTANT,           // [0-9]
    IDENTIFIER,         // [a-Z | 0-9 | _]
    RETURN_SYMBOL,      // ->
    INCLUDE_STATMENT    // #include <[a-Z | "]>

} TokenType;

static TokenType type_specifiers[8] = {
    FLOAT, INT, DOUBLE, LONG, CHAR, VOID, ULONG, BOOL
};


#endif