#include "parser.h"
#include <time.h>

int main(int argc, char **argv) {
    srand(time(NULL));
    // const int token_num = 100;
    // Token **tokens = malloc(sizeof(Token*) * token_num);
    // tokenize("test.txt", tokens, token_num);
#if DEBUG_LEX
    while(*tokens != 0) {
        printf("tokenType: %d, value: %s\n", (*tokens)->type, (*tokens)->string);
        *tokens++;
    }
#endif
    if (argc < 2)
        perror("insufficient arguments provided");
    char arg2[100], arg1[100];
    if (argc == 2) {
        snprintf(arg2, 100, "%s.c", argv[1]);
    } else {
        snprintf(arg2, 100, "%s", argv[2]);
    }

    snprintf(arg1, 100, "%s.lws", argv[1]);

    parse(arg1, arg2);


    return 0;
}