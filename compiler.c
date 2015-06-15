#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#pragma warning(disable: 4996)

#define TRUE                  1
#define FALSE                 0
#define LEXBUF_DEFAULT_SIZE 256

enum lexer_type
{
    lexer_unknown,
    lexer_string,
    lexer_character,
    lexer_literal,
    lexer_number,
    lexer_symbol
};

struct lexer
{
    FILE  *fd;
    int    line;
    int    id;
    int    start;
    int    ch;

    char  *buf;
    int    size;
    int    allocated;
    enum lexer_type type;
};

__inline void
lexer_init(struct lexer **lexer, const char * const filename)
{
    (*lexer) = (struct lexer *)malloc(sizeof(struct lexer));
    (*lexer)->line = 1;
    (*lexer)->id = 1;
    (*lexer)->buf = (char *)malloc(LEXBUF_DEFAULT_SIZE);
    (*lexer)->size = 0;
    (*lexer)->ch = -1;
    (*lexer)->allocated = LEXBUF_DEFAULT_SIZE;
    (*lexer)->fd = fopen(filename, "r");
}

__inline void
lexer_destroy(struct lexer **lexer)
{
    fclose((*lexer)->fd);
    free((*lexer)->buf);
    free((*lexer));
    (*lexer) = NULL;
}

__inline void
lexer_getch(struct lexer *lexer)
{
    lexer->ch = fgetc(lexer->fd);    
}

__inline void
lexer_putch(struct lexer *lexer)
{
    lexer->buf[lexer->size] = lexer->ch;
    lexer->size++;
    lexer->id++;
    if (lexer->size >= lexer->allocated) {
        lexer->allocated += 256;
        lexer->buf = (char *)realloc(lexer->buf, lexer->allocated);
        memset(lexer->buf + lexer->size, 0, lexer->allocated - lexer->size);
    }
}

__inline void
lexer_skipline(struct lexer *lexer)
{
    while (!feof(lexer->fd) && (lexer->ch != '\n')) {
        lexer_getch(lexer);
    }
}

__inline void
lexer_reset(struct lexer *lexer)
{
    memset(lexer->buf, 0, lexer->allocated);
    lexer->size = 0;
    lexer->start = 0;
    lexer->type = lexer_unknown;
}

__inline int
lexer_next(struct lexer *lexer)
{
    lexer_reset(lexer);
    if (feof(lexer->fd))
        return FALSE;
    /** skip whitespace */
    while (!feof(lexer->fd) && isspace(lexer->ch)) {
        if (lexer->ch == '\n') {
            lexer->id = 1;
            lexer->line++;
        }
        lexer_getch(lexer);
    }    

    lexer->start = lexer->id;
    if (isdigit(lexer->ch)) {
        while (!feof(lexer->fd) && isdigit(lexer->ch)) {
            lexer_putch(lexer);
            lexer_getch(lexer);
        }
        lexer->type = lexer_number;
    } else if (isalpha(lexer->ch)) {
        while (!feof(lexer->fd) && (isdigit(lexer->ch) || isalpha(lexer->ch) || lexer->ch == '_')) {
            lexer_putch(lexer);
            lexer_getch(lexer);
        }
        lexer->type = lexer_literal;
    } else if (lexer->ch == '\'' || lexer->ch == '\"') {
        int ch = lexer->ch;
        lexer->type = ch == '\"' ? lexer_string : lexer_character;
        lexer_getch(lexer);
        while (!feof(lexer->fd) && lexer->ch != ch) {
            if (lexer->ch == '\\') {
                lexer_putch(lexer);
                lexer_getch(lexer);
            }
            lexer_putch(lexer);
            lexer_getch(lexer);
        }
        lexer_getch(lexer);
    } else {
        if (lexer->ch == '\n')
            return FALSE;
        lexer_putch(lexer);
        lexer_getch(lexer);
        lexer->type = lexer_symbol;
    }
    return TRUE;
}

static void
parser_preprocessor(struct lexer *lexer)
{
    while (lexer_next(lexer)) {
        if (lexer->type == lexer_symbol && lexer->buf[0] == '#') {
            lexer_skipline(lexer);
        } else {
            break;
        }
    }
}

static void
parser_program(struct lexer *lexer)
{
    lexer_reset(lexer);
    lexer_getch(lexer);

    parser_preprocessor(lexer);

    do {
        fprintf(stdout, "Line: %08d, Column: %03d, Type: %02d, Buffer: %s\n", lexer->line, lexer->start, lexer->type, lexer->buf);
    } while (lexer_next(lexer));
}

int
main(int argc, char **argv)
{
    struct lexer *lexer = NULL;
    

    lexer_init(&lexer, "test.c");
    parser_program(lexer);
    lexer_destroy(&lexer);
    system("pause");
    return 0;
}
