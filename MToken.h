//
//  MToken.h
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//

#ifndef MToken_h
#define MToken_h
#include "TokenType.h"

struct MToken {
    int type; // compatible with TokenType
    const char * value;
    MToken(int, const char *);
    void print();
};

// MToken * initMToken(int type, char * value);

#endif /* MToken_h */
