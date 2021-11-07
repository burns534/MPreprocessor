#include "ASTNodeType.h"
#include "TokenType.h"
#include "map.h"
#include <stdarg.h>
#include <stdio.h>

#ifndef v2parser_h
#define v2parser_h

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode **children;
    size_t child_count, children_size;
    char *value;
} ASTNode;

void print_tree(ASTNode *root);

ASTNode * type();
ASTNode * declaration();
ASTNode * statements();
ASTNode * postfix_expression();
ASTNode * expression();
ASTNode * if_statement();
ASTNode * pattern();

ASTNode * parse(Token **tks, size_t token_count);

#endif
#pragma once