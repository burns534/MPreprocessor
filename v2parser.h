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
    size_t child_count;
    char *value;
} ASTNode;

void print_tree(ASTNode *root);

ASTNode * type();
ASTNode * declaration();
ASTNode * statement();

ASTNode * parse();

#endif
#pragma once