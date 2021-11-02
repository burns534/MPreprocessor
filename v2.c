#include "v2lexer.h"
#include "v2parser.h"

static Token **tokens;
static size_t token_count = 0;

int main (int argc, char **argv) {
    tokens = malloc(TOKEN_MAX * sizeof(Token *));

    if (argc < 2) return 0;

    char filename[100], *fs, *current_filename;
    size_t fs_l;
    snprintf(filename, 100, "%s", argv[1]);
    current_filename = filename;

    FILE *handle = fopen(filename, "r");

    // read file into fs buffer
    if (handle) {
        fseek(handle, 0, SEEK_END);

        fs_l = ftell(handle);

        fs = malloc(fs_l + 1);

        fseek(handle, 0, SEEK_SET);

        if (fs) {
            fread(fs, 1, fs_l, handle);
        }

        fs[fs_l] = 0;

        fclose(handle);

    } else {
        perror("unable to open file");
        exit(1);
    }

    if (fs) {
        lex(fs, fs_l, tokens, &token_count, current_filename);

        // for (int i = 0; i < token_count; i++)
        //     print_token(tokens[i]);

        ASTNode *ast = parse();
        print_tree(ast);
        
    }
    
    return 0;
}