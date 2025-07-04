// Lexer is a classical example of usecase for coroutines.
// This is a *very* simple and basic lexer that
// can lex single digit integers, + and -.

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define GENERATOR_IMPLEMENTATION
#include "generator.h"

typedef enum {
    TK_EOF,
    TK_INT,
    TK_OP,
} TokenKind;

typedef union {
    char tk_op;
    int  tk_int;
} TokenValue;

typedef struct {
    TokenKind  kind;
    TokenValue value;
} Token;

void* lex(void* input_void) {
    if (input_void == NULL) return NULL;

    const char* input = input_void;
    Token token = {0};

    while(true) {
        switch(*input) {
            // Numba
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                token.kind = TK_INT;
                token.value.tk_int = *input - '0';
            } break;

            // Operators
            case '*': case '+': case '-': {
                token.kind = TK_OP;
                token.value.tk_op = *input;
            } break;

            default: {
                token.kind = TK_EOF;
                return NULL;
            }
        }
        input++;

        // For every token we consume, we yield control back to the caller (a parser, I guess).
        yield(&token);
    }

    return NULL;
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        printf("Usage: %s <input-text>\n", argv[0]);
        return 1;
    }

    void* base = malloc(4096);
    Generator g = generator_create(lex, argv[1], base, 4096);

    // Consume those tokens
    bool quit = false;
    foreach (value, &g) {
        if (quit) break;
        Token *token = value;
        switch(token->kind){
            case TK_INT: { printf("TK_INT: %d\n", token->value.tk_int); } break;
            case TK_OP:  { printf("TK_OP:  %c\n", token->value.tk_op);  } break;
            default:     { printf("Done!\n"); quit = true;              } break;
        }
    }

    free(generator_destroy(&g));

    return 0;
}
