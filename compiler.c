#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#pragma warning(disable: 4996)

enum TT
{
    T_START,
    T_SYMBOL,
    T_NUMBER,
    T_STRING,
    T_LITERAL
};

struct token
{
    int           start;
    int           size;
    enum TT       type;
    struct token *next;
    struct token *prev;
};

struct tokenbuf
{
    FILE         *fd;
    int           id;
    int           ch;
    char         *buf;
    int           allocated;
    int           readed;
};

__inline void
tokenbuf_clean(struct tokenbuf *buf)
{
    if (buf->buf)
        free(buf->buf);
    buf->buf = NULL;
    buf->readed = 0;
    buf->allocated = 0;
    buf->ch = -1;
    buf->id = 0;
    if (buf->fd)
        fclose(buf->fd);
    buf->fd = NULL;
}

__inline int
tokenbuf_eof(struct tokenbuf *buf)
{
    return  buf->id >= buf->readed && feof(buf->fd);
}

__inline void 
tokenbuf_next(struct tokenbuf *buf)
{
    if (buf->allocated <= buf->id) {
        buf->allocated += 512;
        if (buf->buf == NULL) {
            buf->buf = (char *)malloc(buf->allocated);
        } else {
            buf->buf = (char *)realloc(buf->buf, buf->allocated);
        }        
        memset(buf->buf + buf->id, 0, buf->allocated - buf->id);
        buf->readed = fread(buf->buf + buf->id, 1, 512, buf->fd);
        buf->readed = buf->id + buf->readed;
    }
    buf->ch = buf->buf[buf->id];
    buf->id++;
}

static void 
token_create(struct token **tok, int start, int size, enum TT type)
{
    (*tok) = (struct token *)malloc(sizeof(struct token));
    (*tok)->start = start;
    (*tok)->size = size;
    (*tok)->type = type;
    (*tok)->next = (*tok);
    (*tok)->prev = (*tok);
}

static void
token_remove(struct token **tok)
{
    struct token *prev = (*tok)->prev;
    struct token *next = (*tok)->next;
    if ((*tok) != next && prev != next) {
        next->prev = prev;
        prev->next = next;
    }
    free((*tok));
    (*tok) = NULL;
}

static struct token *
token_add(struct token **tok, int start, int size, enum TT type)
{
    struct token *created;
    token_create(&created, start, size, type);
    created->next = (*tok);
    created->prev = (*tok)->prev;
    (*tok)->prev->next = created;
    (*tok)->prev = created; 
    return created;
}

static void
token_free(struct token **tok)
{
}

static void
token_parse_number(struct token **tok, struct tokenbuf *buf)
{
    int start = buf->id;
    while (!tokenbuf_eof(buf) && isdigit(buf->ch)) {
        tokenbuf_next(buf);
    }
    token_add(tok, start, buf->id - start, T_NUMBER);
}

static void
token_parse_literal(struct token **tok, struct tokenbuf *buf)
{
    int start = buf->id;
    while (!tokenbuf_eof(buf) && (isdigit(buf->ch) || isalpha(buf->ch) || buf->ch == '_')) {
        tokenbuf_next(buf);
    }
    token_add(tok, start, buf->id - start, T_LITERAL);
}

static void
token_parse_string(struct token **tok, struct tokenbuf *buf)
{
    int start = buf->id + 1;
    tokenbuf_next(buf);
    while (!tokenbuf_eof(buf) && (buf->ch != '\"')) {
        tokenbuf_next(buf);
    }
    token_add(tok, start, buf->id - start, T_STRING);
    tokenbuf_next(buf);
}

static void
token_parse_skipws(struct tokenbuf *buf)
{
    while (!tokenbuf_eof(buf) && (isspace(buf->ch))) {
        tokenbuf_next(buf);
    }
}

static void
token_parse(struct token **tok, struct tokenbuf *buf)
{
    tokenbuf_next(buf);
    while (!tokenbuf_eof(buf)) {
        token_parse_skipws(buf);
        if (isdigit(buf->ch)) {
            token_parse_number(tok, buf);
        } else if (isalpha(buf->ch)) {
            token_parse_literal(tok, buf);
        } else if (buf->ch == '\"') {
            token_parse_string(tok, buf);
        } else {
            token_add(tok, buf->id, 1, T_SYMBOL);
            tokenbuf_next(buf);
        }
    }
}

int
main(int argc, char **argv)
{
    FILE *fd;
    struct token *result = NULL;
    

    fd = fopen("test.c", "r");
    if (fd != NULL) {
        struct tokenbuf buf = { NULL, 0, EOF, NULL, 0 };

        buf.fd = fd;
        token_create(&result, 0, 0, T_START);
        token_parse(&result, &buf);
        fclose(fd);
        tokenbuf_clean(&buf);
    }
    token_free(&result);
    return 0;
}
