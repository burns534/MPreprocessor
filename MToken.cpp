//
//  MToken.c
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//

#include "MToken.h"

// MToken * initMToken(int type, char * value) {
//     MToken *token = malloc(sizeof(MToken));
//     token->type = type;
//     token->value = value;
//     return token;
// }

MToken::MToken(int type, const char *value) {
    this->type = type;
    this->value = value;
}

void MToken::print() {
    printf("%d/%s ", type, value);
}
