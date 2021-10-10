#ifndef TOKEN_H
#define TOKEN_H
#pragma once
#include "TokenType.h"
#include <stdlib.h>

typedef struct {
    TokenType type;
    const char *string;
} Token;

Token * create_token(TokenType, char*);

#endif