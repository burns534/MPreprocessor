#include "Token.h"

Token * create_token(TokenType type, char *string) {
    Token *result = malloc(sizeof(Token));
    result->type = type;
    result->string = string;
    return result;
}