#ifndef TOKENIZER_H
#define TOKENIZER_H
#pragma once
#include <stdio.h>
#include <string.h>
#include "Token.h"

#define TOKEN_MAX_LENGTH 512

/**
 * Accepts file name, token array, and token array length and populates array or returns -1 for error
 */
int tokenize(const char *, Token **, int);
#endif