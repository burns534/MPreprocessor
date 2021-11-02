#include "TokenType.h"
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef v2lexer_h
#define v2lexer_h

#define LITERAL_BUFFER 4096
#define TOKEN_MAX ((unsigned long) 1UL << 20) // about 8 mb allocated, should be enough
#define IDENTIFIER_MAX 256
#define DECIMAL_MAX 64 // for now, will change this when big int support is added
#define BINARY_MAX 64
#define KEYWORD_MAX 32
#define OPERATOR_MAX 4

void lex(char *filestring, size_t length, Token **tks, size_t *tkl, char *filename);

#endif
#pragma once