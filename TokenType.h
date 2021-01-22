//
//  TokenType.h
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//

#ifndef TokenType_h
#define TokenType_h
#define NUM_TOKENS 41

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum {
    NONE,
    IDENTIFIER = 255,
    CONSTANT,
    STRING_LITERAL,
    // operators
    SIZEOF,
    PTR_OP,
    INC_OP,
    DEC_OP,
    LEFT_OP,
    RIGHT_OP,
    LE_OP,
    GE_OP,
    EQ_OP,
    NE_OP,
    AND_OP,
    OR_OP,
    MUL_ASSIGN,
    DIV_ASSIGN,
    MOD_ASSIGN,
    ADD_ASSIGN,
    SUB_ASSIGN,
    LEFT_ASSIGN,
    RIGHT_ASSIGN,
    AND_ASSIGN,
    XOR_ASSIGN,
    OR_ASSIGN,
    ELLIPSES,
    TYPE_NAME,
    CLASS_NAME,
    // storage modifiers
    TYPEDEF,
    EXTERN,
    STATIC,
    AUTO,
    REGISTER,
    // type modifiers (const/volatile are type qualifiers)
    CHAR,
    SHORT,
    INT,
    LONG,
    SIGNED,
    UNSIGNED,
    FLOAT,
    DOUBLE,
    BOOL,
    CONST,
    VOLATILE,
    VOID,
    OVERRIDE,
    PRIVATE,
    // keywords
    SELF,
    SUPER,
    INIT,
    DEINIT,
    STRUCT,
    UNION,
    ENUM,
    CLASS,
    PROTOCOL,
    // control-flow
    CASE,
    DEFAULT,
    IF,
    ELSE,
    SWITCH,
    WHILE,
    DO,
    FOR,
    GOTO,
    CONTINUE,
    BREAK,
    RETURN
} TokenType;



#endif /* TokenType_h */
