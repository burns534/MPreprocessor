//
//  Lexer.h
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//

#ifndef Lexer_h
#define Lexer_h
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include "MToken.h"
#define DEFAULT_TOKEN_BUFFER_SIZE 1024
#define MAX_IDENTIFIER_LENGTH 128
#define MAX_NUMBER_LENGTH 64
#define DEFAULT_STRING_BUFFER_LENGTH 128

MToken ** lex(FILE *, size_t *);
MToken ** lex(const char *string, size_t *token_count);
const char * stringRepresentation(MToken *);

#endif /* Lexer_h */
