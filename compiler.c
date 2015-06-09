#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#pragma warning(disable: 4996)

#define BUFFER_BULK_SIZE 256
#define BUFFER_READ_SIZE 256

#if !defined(BUFFER_FILE_STRUCT)
struct Buffer
{
    uint8_t * data;
    size_t    allocated;
    size_t    size;
    size_t    it;
};
#else
struct Buffer
{
    FILE   *  fd;
    size_t    size;
    size_t    it;
};
#endif

enum TokenType
{
    T_START,
    T_STRING,
    T_NUMBER,
    T_LITERAL,
    T_SYMBOL
};

struct Token
{
    size_t         offset;
    size_t         size;
    enum TokenType type;
};

/** Linked list */
struct Node
{
    void        * data;
    struct Node * next;
};

__inline struct Node *
listCreate(void *data)
{
    struct Node *ret = (struct Node *)malloc(sizeof(struct Node));
    ret->data = data;
    ret->next = NULL;
    return ret;
}

__inline struct Node *
listAppend(struct Node *node, void * data)
{
    struct Node *ret = listCreate(data);
    while (node->next != NULL)
        node = node->next;
    node->next = ret;
    return ret;
}

__inline void
listRemove(struct Node *node, void (__cdecl * pfnFree)(void *memory))
{
    struct Node *temp = node;

    if (temp == NULL)
        return;
    if (pfnFree != NULL) {
        do {
            if (temp->data)
                (*pfnFree)(temp->data);
        } while ((temp = temp->next) != NULL);
        temp = node;
    }
    do {
        node = node->next;
        free(temp);
        temp = node;
    } while ( node != NULL );
}

/** Buffer */
static void bufferCreate(struct Buffer **buffer, size_t size);
static void bufferDelete(struct Buffer **buffer);
static void bufferAppend(struct Buffer *buffer, const uint8_t *data, size_t size);
static void bufferInsert(struct Buffer *buffer, size_t start, const uint8_t *data, size_t size);
static void bufferReadName(struct Buffer **buffer, const char * const filename);
static void bufferReadFile(struct Buffer **buffer, FILE *fd);
/** Lexer */
__inline int 
lexer_eof(struct Buffer *buffer)
{
    return buffer->it >= buffer->size;
}

__inline int 
lexer_ch(struct Buffer *buffer)
{
    return (int)buffer->data[buffer->it];
}

__inline int 
lexer_isdigit(struct Buffer *buffer)
{
    return isdigit( lexer_ch(buffer) );
}

__inline int 
lexer_isalpha(struct Buffer *buffer)
{
    return isalpha( lexer_ch(buffer) );
}

__inline int 
lexer_isspace(struct Buffer *buffer)
{
    return isspace( lexer_ch(buffer) );
}

__inline int 
lexer_next(struct Buffer *buffer)
{
    ++buffer->it;
    return lexer_eof(buffer);
}

__inline void 
lexer_skipws(struct Buffer *buffer)
{
    while (!lexer_eof(buffer) && lexer_isspace(buffer)) {
        lexer_next(buffer);
    }
}

__inline void 
lexer_number(struct Buffer *buffer, struct Token *token)
{
    lexer_next(buffer);
}

__inline void
lexer_literal(struct Buffer *buffer, struct Token *token)
{
    lexer_next(buffer);
}

__inline void
lexer_string(struct Buffer *buffer, struct Token *token)
{
    lexer_next(buffer);
}

static void
lexer(struct Buffer *buffer, struct Node **tokens)
{
    struct Token * start = (struct Token *)malloc(sizeof(struct Token));
    start->offset = 0;
    start->size = 0;
    start->type = T_START;
    (*tokens) = listCreate(start);
    buffer->it = 0;
    while (!lexer_eof(buffer)) {
        lexer_skipws(buffer);
        struct Token *token = (struct Token *)malloc(sizeof(struct Token));
        if (lexer_isdigit(buffer)) {
            lexer_number(buffer, token);
        } else if (lexer_isalpha(buffer)) {
            lexer_literal(buffer, token);
        } else {
            int ch = lexer_ch(buffer);
            if (ch == '"') {
                lexer_string(buffer, token);
            } else {
                lexer_next(buffer);
            }
        }
        listAppend((*tokens), token);
        
    }
}

/***********/

static struct Buffer *
lookupFile(const char* const lpszFileName, struct Node *paths);

static void
lookupInclude(struct Node *paths, const char * const filename, FILE **fd);

int
main(int argc, char **argv)
{
    struct Buffer *readed = NULL;
    struct Node   *paths  = NULL;
    struct Node   *tokens = NULL;

    paths = listCreate(".");
    listAppend(paths, "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\include");
    readed = lookupFile("compiler.c", paths);
    lexer(readed, &tokens);
    listRemove(tokens, free);
    listRemove(paths, NULL);
    bufferDelete(&readed);
    return 0;
}

static struct Buffer *
lookupFile(const char* const lpszFileName, struct Node *paths)
{
    struct Buffer *buffer = NULL;    
    FILE *fd = NULL;
    bufferReadName(&buffer, lpszFileName);
#if 1
    lookupInclude(paths, "stdio.h", &fd);
    if (fd)
        fclose(fd);
#endif
    return buffer;
}

static void
lookupInclude(struct Node *paths, const char * const filename, FILE **fd)
{
    struct Node *temp = paths;
    char *apath;
    size_t flen = strlen(filename);
    if (fd == NULL || paths == NULL)
        return;
    do {
        size_t size = strlen((const char *)temp->data) + flen + 5;
        apath = (char *)malloc(size);
        memset(apath, 0, size);
        strcpy(apath, (const char *)temp->data);
        strcat(apath, "/");
        strcat(apath, filename);
        (*fd) = fopen(apath, "r");
        free(apath);
        apath = NULL;
        if ((*fd))
            return;
    } while ((temp = temp->next) != NULL);
}

static void 
bufferCreate(struct Buffer **buffer, size_t size)
{
    if (buffer) {
        (*buffer) = (struct Buffer *)malloc(sizeof(struct Buffer));
        (*buffer)->size = 0;
        (*buffer)->allocated = size;
        (*buffer)->it = 0;
        (*buffer)->data = (uint8_t *)malloc(size);
    }
}

static void
bufferDelete(struct Buffer **buffer)
{
    if (buffer) {
        if ((*buffer)) {
            if ((*buffer)->data)
                free((*buffer)->data);
            (*buffer)->data = NULL;
            (*buffer)->size = 0;
            (*buffer)->allocated = 0;
        }
        (*buffer) = NULL;
    }
}

static void 
bufferAppend(struct Buffer *buffer, const uint8_t *data, size_t size)
{
    if (buffer == NULL || data == NULL)
        return;
    if (buffer->allocated < buffer->size + size) {
        buffer->allocated += size + BUFFER_BULK_SIZE;
        buffer->data = (uint8_t *)realloc(buffer->data, buffer->allocated);
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    /**Empty tail*/
    memset(buffer->data + buffer->size, 0, buffer->allocated - buffer->size);
}

static void 
bufferInsert(struct Buffer *buffer, size_t start, const uint8_t *data, size_t size)
{
    size_t it;
    if (buffer == NULL || data == NULL || start > buffer->size)
        return;
    buffer->allocated += size;
    buffer->data = (uint8_t *)realloc(buffer->data, buffer->allocated);
    for (it = buffer->size; it >= start; --it) {
        buffer->data[it + size] = buffer->data[it];
    }
    memcpy(buffer->data + start, data, size);
    buffer->size += size;
}

static void 
bufferReadName(struct Buffer **buffer, const char * const filename)
{
    FILE *fd;
    if (buffer == NULL || filename == NULL)
        return;    
    fd = fopen(filename, "r");
    if (fd) {
        bufferReadFile(buffer, fd);
        fclose(fd);
    } else {
        fprintf(stderr, "File %s not found\n", filename);
    }
}

static void
bufferReadFile(struct Buffer **buffer, FILE *fd)
{
    size_t  readed;
    uint8_t fbuf[BUFFER_READ_SIZE];
    bufferDelete(buffer);
    bufferCreate(buffer, BUFFER_BULK_SIZE);
    while ((readed = fread(fbuf, 1, BUFFER_READ_SIZE, fd)) > 0) {
        bufferAppend((*buffer), fbuf, readed);
    }
}
