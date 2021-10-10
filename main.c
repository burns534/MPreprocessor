#include "parser.h"

int main(int argc, char **argv) {
    // const int token_num = 100;
    // Token **tokens = malloc(sizeof(Token*) * token_num);
    // tokenize("test.txt", tokens, token_num);
#if DEBUG_LEX
    while(*tokens != 0) {
        printf("tokenType: %d, value: %s\n", (*tokens)->type, (*tokens)->string);
        *tokens++;
    }
#endif

    parse("test.txt", "output.c");


    return 0;
}