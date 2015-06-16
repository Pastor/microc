#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma warning(disable: 4996)

#define TRUE                  1
#define FALSE                 0


/** Lexer */
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

__inline int
lexer_match(struct lexer *lexer, const char * const text)
{
    return !strcmp(lexer->buf, text);
}

__inline char *
lexer_strdup(struct lexer *lexer)
{
    return strdup(lexer->buf);
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
    lexer_putch(lexer);
    while (!feof(lexer->fd) && (lexer->ch != '\n')) {
        lexer_getch(lexer);
        lexer_putch(lexer);
    }
}

__inline void
lexer_go(struct lexer *lexer, char ch)
{
    while (!feof(lexer->fd) && (lexer->ch != ch)) {
        lexer_getch(lexer);
        lexer_putch(lexer);
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
        if (lexer->ch == EOF)
            return FALSE;
        lexer_putch(lexer);
        lexer_getch(lexer);
        lexer->type = lexer_symbol;
    }
    return TRUE;
}
/** */
struct variable
{
    char  *type;
    char  *name;
    char  *value;
    int    isunsigned;
    int    pointer;
    int    external;
};

struct decl_type {
    char  *name;
    int    pointer;
};

struct fun_parm
{
    char  *type;
    char  *name;
    int    pointer;
    int    isunsigned;
};

struct function
{
    char  *type;
    char  *name;
    int    external;
    int    isunsigned;
    int    pointer;
    char  *dll;

    struct fun_parm *parms;
    int              parmc;
};

struct context
{
    struct variable  *vars;
    int               varc;

    struct function  *funs;
    int               func;

    struct decl_type *decs;
    int               decc;

    struct context   *parent;

    char             *name;
};

struct codegen
{
    FILE   *fd;

    struct context *ctx;
};

__inline int
context_hastype(struct context *ctx, const char * const name)
{
    int i;
    for (i = 0; i < ctx->decc; ++i) {
        if (!strcmp(ctx->decs[i].name, name))
            return TRUE;
    }
    if (ctx->parent)
        return context_hastype(ctx->parent, name);
    return FALSE;
}

__inline struct variable *
context_hasvar(struct context *ctx, const char * const name)
{
    int i;
    for (i = 0; i < ctx->varc; ++i) {
        if (!strcmp(ctx->vars[i].name, name))
            return &ctx->vars[i];
    }
    if (ctx->parent)
        return context_hasvar(ctx->parent, name);
    return NULL;
}

__inline struct function *
context_hasfun(struct context *ctx, const char * const name)
{
    int i;
    for (i = 0; i < ctx->func; ++i) {
        if (!strcmp(ctx->funs[i].name, name))
            return &ctx->funs[i];
    }
    if (ctx->parent)
        return context_hasfun(ctx->parent, name);
    return NULL;
}

__inline struct variable *
context_varadd(struct context *ctx, const char * const type, const char * const name, const char * const value, int external)
{
    if (ctx->vars == NULL && ctx->varc == 0) {
        ctx->vars = (struct variable *)malloc(sizeof(struct variable));
    } else {
        ctx->vars = (struct variable *)realloc(ctx->vars, (ctx->varc * sizeof(struct variable)) + sizeof(struct variable));
    }
    memset((unsigned char *)ctx->vars + (ctx->varc * sizeof(struct variable)), 0, sizeof(struct variable));
    ctx->vars[ctx->varc].type = strdup(type);
    ctx->vars[ctx->varc].name = strdup(name);
    ctx->vars[ctx->varc].pointer = FALSE;
    ctx->vars[ctx->varc].value = NULL;
    ctx->vars[ctx->varc].external = external;
    ctx->vars[ctx->varc].isunsigned = FALSE;
    if (value != NULL)
        ctx->vars[ctx->varc].value = strdup(value);
    ctx->varc++;
    return &ctx->vars[ctx->varc - 1];
}

__inline void
context_varclean(struct context *ctx)
{
    int i;

    for (i = 0; i < ctx->varc; ++i) {
        free(ctx->vars[i].type);
        free(ctx->vars[i].name);
        if (ctx->vars[i].value)
            free(ctx->vars[i].value);
    }
    free(ctx->vars);
    ctx->vars = NULL;
    ctx->varc = 0;
}

__inline void
context_typeadd(struct context *ctx, const char * const name, int pointer)
{
    if (ctx->decs == NULL && ctx->decc == 0) {
        ctx->decs = (struct decl_type *)malloc(sizeof(struct decl_type));
    } else {
        ctx->decs = (struct decl_type *)realloc(ctx->decs, (ctx->decc * sizeof(struct decl_type)) + sizeof(struct decl_type));
    }
    memset((unsigned char *)ctx->decs + (ctx->decc * sizeof(struct decl_type)), 0, sizeof(struct decl_type));
    ctx->decs[ctx->decc].pointer = pointer;
    ctx->decs[ctx->decc].name = strdup(name);
    ctx->decc++;
}

__inline void
context_typeclean(struct context *ctx)
{
    int i;

    for (i = 0; i < ctx->decc; ++i) {
        free(ctx->decs[i].name);
    }
    free(ctx->decs);
    ctx->decs = NULL;
    ctx->decc = 0;
}

__inline struct function *
context_funadd(struct context *ctx, const char * const name, const char * const type, int pointer, int isunsigned)
{
    if (ctx->funs == NULL && ctx->func == 0) {
        ctx->funs = (struct function *)malloc(sizeof(struct function));
    } else {
        ctx->funs = (struct function *)realloc(ctx->funs, (ctx->func * sizeof(struct function)) + sizeof(struct function));
    }
    memset((unsigned char *)ctx->funs + (ctx->func * sizeof(struct function)), 0, sizeof(struct function));
    ctx->funs[ctx->func].external = FALSE;
    ctx->funs[ctx->func].name = strdup(name);
    ctx->funs[ctx->func].type = strdup(type);
    ctx->funs[ctx->func].pointer = pointer;
    ctx->funs[ctx->func].isunsigned = isunsigned;
    ctx->func++;
    return &ctx->funs[ctx->func - 1];
}

__inline void
context_funaddparm(struct function *fun, const char * const type, const char * const name, int pointer, int isunsigned)
{
    if (fun->parms == NULL && fun->parmc == 0) {
        fun->parms = (struct fun_parm *)malloc(sizeof(struct fun_parm));
    } else {
        fun->parms = (struct fun_parm *)realloc(fun->parms, (fun->parmc * sizeof(struct fun_parm)) + sizeof(struct fun_parm));
    }
    memset((unsigned char *)fun->parms + (fun->parmc * sizeof(struct fun_parm)), 0, sizeof(struct fun_parm));
    fun->parms[fun->parmc].name = strdup(name);
    fun->parms[fun->parmc].type = strdup(type);
    fun->parms[fun->parmc].isunsigned = isunsigned;
    fun->parms[fun->parmc].pointer = pointer;
    fun->parmc++;
}

__inline void
context_paramclean(struct function *fun)
{
    int i;

    for (i = 0; i < fun->parmc; ++i) {
        free(fun->parms[i].type);
        free(fun->parms[i].name);
    }
    free(fun->parms);
    fun->parms = NULL;
    fun->parmc = 0;
}

__inline void
context_funclean(struct context *ctx)
{
    int i;

    for (i = 0; i < ctx->func; ++i) {
        context_paramclean(&ctx->funs[i]);
        free(ctx->funs[i].name);
        free(ctx->funs[i].type);
        if (ctx->funs[i].dll)
            free(ctx->funs[i].dll);
    }
    free(ctx->funs);
    ctx->funs = NULL;
    ctx->func = 0;
}

__inline 
context_create(struct context **ctx, const char * const name, struct context *parent)
{
    (*ctx) = (struct context *)malloc(sizeof(struct context));
    memset((*ctx), 0, sizeof(struct context));
    (*ctx)->parent = parent;
    (*ctx)->name = strdup(name);
}

__inline
context_destroy(struct context **ctx)
{
    context_varclean((*ctx));
    context_typeclean((*ctx));
    context_funclean((*ctx));
    if ((*ctx)->name)
        free((*ctx)->name);
    free((*ctx));
    (*ctx) = NULL;
}

static void
codegen_init(struct codegen **codegen, const char * const filename)
{
    struct function *fun;

    (*codegen) = (struct codegen *)malloc(sizeof(struct codegen));
    (*codegen)->fd = fopen(filename, "w");
    context_create(&(*codegen)->ctx, "global", NULL);
#if defined(_MSC_VER)
    fprintf((*codegen)->fd, "format PE console\n");
    fprintf((*codegen)->fd, "entry start\n");
    fprintf((*codegen)->fd, "include 'win32a.inc'\n\n");
    fprintf((*codegen)->fd, "section '.code' code readable writable executable\n");
#else
    fprintf((*codegen)->fd, "format ELF console\n");
    fprintf((*codegen)->fd, "entry start\n");
    fprintf((*codegen)->fd, "segment readable executable\n");
#endif
    fprintf((*codegen)->fd, "start:\n");

    /** Define global type */
    context_typeadd((*codegen)->ctx, "char", 0);
    context_typeadd((*codegen)->ctx, "int", 0);
    context_typeadd((*codegen)->ctx, "float", 0);
    context_typeadd((*codegen)->ctx, "long", 0);
    context_typeadd((*codegen)->ctx, "void", 0);

    /** Define library fun */
    fun = context_funadd((*codegen)->ctx, "malloc", "void", 1, FALSE); //malloc
    fun->external = TRUE;
    fun->dll = strdup("msvcr120d.dll");
    context_funaddparm(fun, "long", "size", 0, TRUE);
    fun = context_funadd((*codegen)->ctx, "free", "void", 0, FALSE); //free
    fun->external = TRUE;
    fun->dll = strdup("msvcr120d.dll");
    context_funaddparm(fun, "void", "pointer", 1, FALSE);
    fun = context_funadd((*codegen)->ctx, "fclose", "void", 1, FALSE); //fclose
    fun->external = TRUE;
    fun->dll = strdup("msvcr120d.dll");
    context_funaddparm(fun, "char", "name", 1, FALSE);
    context_funaddparm(fun, "char", "mode", 1, FALSE);
}

static void
codegen_destroy(struct codegen **codegen)
{
    context_destroy(&(*codegen)->ctx);
    fclose((*codegen)->fd);
    free((*codegen));
    (*codegen) = NULL;
}

static void
preprocessor(struct lexer *lexer, struct codegen *codegen)
{
    while (lexer_next(lexer)) {
        if (lexer->type == lexer_symbol && lexer->buf[0] == '#') {
            lexer_putch(lexer);
            lexer_skipline(lexer);
            //fprintf(codegen->fd, ";");
            //fprintf(codegen->fd, lexer->buf);
            lexer_reset(lexer);
        } else {
            break;
        }
    }
    fprintf(codegen->fd, "\n");
}

__inline char *
decl_gettype(struct lexer *lexer, struct context *ctx, int * pointer, int * isunsigned)
{
    char *type;

    if (lexer_match(lexer, "const"))
        lexer_next(lexer);
    (*pointer) = 0;
    (*isunsigned) = TRUE;
    if (lexer_match(lexer, "unsigned")) {
        (*isunsigned) = TRUE;
        lexer_next(lexer);
    }    
    if (!context_hastype(ctx, lexer->buf)) {
        fprintf(stderr, "Type %s not declared\n", lexer->buf);
        return NULL;
    }
    type = lexer_strdup(lexer);
    lexer_next(lexer);
    while (lexer_match(lexer, "*")) {
        (*pointer)++;
        lexer_next(lexer);
    }
    if (lexer_match(lexer, "const")) {
        lexer_next(lexer);
    }
    return type;
}

static void
decl0(struct lexer *lexer, struct codegen *codegen);

/** eax -> result expression */
static void
expr0(struct lexer *lexer, struct codegen *codegen)
{
    struct variable *var;
    struct function *fun;

    var = context_hasvar(codegen->ctx, lexer->buf);
    if (var != NULL) {
        /** mov eax, [variable]  */
        lexer_next(lexer);
        if (lexer_match(lexer, "=")) {
            lexer_next(lexer);
            expr0(lexer, codegen);
            fprintf(codegen->fd, "    mov  [%s], eax\n", var->name);
        } else {
            fprintf(codegen->fd, "    mov  eax, [%s]\n", var->name);
        }
        expr0(lexer, codegen);
    }
    fun = context_hasfun(codegen->ctx, lexer->buf);
    if (fun != NULL) {
        lexer_next(lexer);
        if (lexer_match(lexer, "(")) {
            lexer_next(lexer);
            if (!lexer_match(lexer, ")"))
                do {
                    expr0(lexer, codegen);
                    /** push eax */
                    fprintf(codegen->fd, "    push eax\n");
                    if (lexer_match(lexer, ",")) {
                        continue;
                    }
                } while (!lexer_match(lexer, ")") && lexer_next(lexer));
        }
        lexer_next(lexer);
        /** call [function] */
        fprintf(codegen->fd, "    call dword [%s]\n", fun->name);
        expr0(lexer, codegen);
    }

    if (lexer_match(lexer, "+") || lexer_match(lexer, "-") || lexer_match(lexer, "/") || lexer_match(lexer, "%") || lexer_match(lexer, "*")) {
        char ch = (char)lexer->buf[0];
        /** push eax */
        lexer_next(lexer);
        if (lexer_match(lexer, "/") && lexer->type == lexer_symbol && ch == '/') {
            lexer_next(lexer);
            lexer_skipline(lexer);
            //fprintf(codegen->fd, ";Origin line: %d, Text: %s\n", lexer->line, lexer->buf);
            lexer_reset(lexer);
            lexer_next(lexer);
        } else if (lexer_match(lexer, "*") && lexer->type == lexer_symbol) {
            do {
                lexer_go(lexer, '*');
                lexer_next(lexer);
            } while (!lexer_match(lexer, "/"));
            lexer_reset(lexer);
            lexer_next(lexer);
            expr0(lexer, codegen);
        } else {
            fprintf(codegen->fd, "    push eax\n");
            expr0(lexer, codegen);
            //fprintf(codegen->fd, "    mov  ecx, eax\n");
            fprintf(codegen->fd, "    pop  ecx\n");
            /** mov  ecx, eax*/
            /** pop  eax */
            switch (ch) {
            case '+': {
                /** add  ecx */
                fprintf(codegen->fd, "    add  ecx, eax\n");
                break;
            }
            case '-': {
                /** sub  ecx */
                fprintf(codegen->fd, "    sub  ecx, eax\n");
                break;
            }
            case '%':
            case '/': {
                /** div  ecx */
                fprintf(codegen->fd, "    div  ecx, eax\n");
                break;
            }
            case '*': {
                /** imul  ecx */
                fprintf(codegen->fd, "    imul ecx, eax\n");
                break;
            }
            }
            fprintf(codegen->fd, "    mov  eax, ecx\n");
            expr0(lexer, codegen);
        }
        
    } else if (lexer->type == lexer_character) {
        /** mov eax, character */
        fprintf(codegen->fd, "    mov  eax, '%c'\n", lexer->buf[0]);
        lexer_next(lexer);
        expr0(lexer, codegen);
    } else if (lexer->type == lexer_string) {
        /** mov eax, [string constant] */
        lexer_next(lexer);
        expr0(lexer, codegen);
    } else if (lexer->type == lexer_number) {
        /** mov eax, number */
        fprintf(codegen->fd, "    mov  eax, %s\n", lexer->buf);
        lexer_next(lexer);
        expr0(lexer, codegen);        
    }
}

static void
stmt2(struct lexer *lexer, struct codegen *codegen)
{
    if (lexer_match(lexer, "return")) {
        lexer_next(lexer);
        expr0(lexer, codegen);
        /** eax -> result */
        fprintf(codegen->fd, "    mov  esp, ebp\n");
        fprintf(codegen->fd, "    pop  ebp\n");
        fprintf(codegen->fd, "    ret\n");
    }
}

static void
stmt1(struct lexer *lexer, struct codegen *codegen)
{
    if (lexer_match(lexer, ";"))
        lexer_next(lexer);
}

static void
stmt0(struct lexer *lexer, struct codegen *codegen, const char * const name)
{
    struct context *ctx;

    if (!lexer_match(lexer, "{"))
        return;
    
    /** Do */
    lexer_next(lexer);
    if (!lexer_match(lexer, "}")) {
        /** Create context  */
        context_create(&ctx, name, codegen->ctx);
        codegen->ctx = ctx;
        decl0(lexer, codegen);
        do {
           stmt1(lexer, codegen);
           stmt2(lexer, codegen); 
           expr0(lexer, codegen);
        } while (!lexer_match(lexer, "}"));
        ctx = codegen->ctx->parent;
        context_destroy(&codegen->ctx);
        codegen->ctx = ctx;        
    } 
    lexer_next(lexer);
}

static void
decl0(struct lexer *lexer, struct codegen *codegen)
{
    if (strcmp(codegen->ctx->name, "global")) {
        fprintf(codegen->fd, "    jmp  decl_%s_end\n", codegen->ctx->name);
        fprintf(codegen->fd, "decl_%s_start:\n", codegen->ctx->name);
    }    
    do {
        char *decl_type;
        char *decl_name;
        int   external = FALSE;
        int   pointer  = 0;
        int   isunsigned = FALSE;
        struct variable *var;
        /** Skip */
        if (lexer_match(lexer, "static") || lexer_match(lexer, "__inline"))
            lexer_next(lexer);
        /** Extern */
        if (lexer_match(lexer, "extern")) {
            external = TRUE;
            lexer_next(lexer);
        }
        decl_type = decl_gettype(lexer, codegen->ctx, &pointer, &isunsigned);
        if (decl_type == NULL) {
            if (strcmp(codegen->ctx->name, "global")) {
                fprintf(codegen->fd, "decl_%s_end:\n", codegen->ctx->name);
            }
            return;
        }
        decl_name = lexer_strdup(lexer);
        lexer_next(lexer);
        if (lexer_match(lexer, "=") && lexer->type == lexer_symbol) {
            /** Declare variables with value */
            lexer_next(lexer);
            expr0(lexer, codegen);
            fprintf(codegen->fd, "    %s dd 0\n", decl_name);
            fprintf(codegen->fd, "    mov [%s], eax\n", decl_name);
            var = context_varadd(codegen->ctx, decl_type, decl_name, NULL, external);
            var->pointer = pointer;
            var->isunsigned = isunsigned;
            
            free(decl_type);
            free(decl_name);
            lexer_next(lexer);
        } else if (lexer_match(lexer, ";") && lexer->type == lexer_symbol) {
            /** Declare variables */
            fprintf(codegen->fd, "    %s dd ?\n", decl_name);
            var = context_varadd(codegen->ctx, decl_type, decl_name, NULL, external);
            var->pointer = pointer;
            var->isunsigned = isunsigned;
            free(decl_type);
            free(decl_name);
            lexer_next(lexer);
        } else if (lexer_match(lexer, "(") && lexer->type == lexer_symbol) {
            /** Declare functions */
            struct function *fun = context_funadd(codegen->ctx, decl_name, decl_type, pointer, isunsigned);
            free(decl_type);
            free(decl_name);
            lexer_next(lexer);
            if (lexer_match(lexer, ")") && lexer->type == lexer_symbol) {
                /** Without params */
            } else {
                do {
                    int pointer = 0;
                    int isunsigned = FALSE;
                    char *param_type = decl_gettype(lexer, codegen->ctx, &pointer, &isunsigned);
                    char *param_name = lexer_strdup(lexer);
                    context_funaddparm(fun, param_type, param_name, pointer, isunsigned);
                    free(param_type);
                    free(param_name);
                    lexer_next(lexer);
                    if (lexer_match(lexer, ",")) {
                        lexer_next(lexer);
                        continue;
                    }
                } while (!lexer_match(lexer, ")") && lexer->type != lexer_symbol);
            }
            lexer_next(lexer);
            if (lexer_match(lexer, ";")) {
                
            } else if (lexer_match(lexer, "{")) {
                /** Function body */
                fprintf(codegen->fd, "%s:\n", fun->name);
                fprintf(codegen->fd, "    push ebp\n");
                fprintf(codegen->fd, "    mov  ebp, esp\n");
                stmt0(lexer, codegen, fun->name);
                fprintf(codegen->fd, "    mov  esp, ebp\n");
                fprintf(codegen->fd, "    pop  ebp\n");
                fprintf(codegen->fd, "    ret\n");
                continue;
            } else {
                fprintf(stderr, "Error declare function");
                break;
            }
        }
    } while (!feof(lexer->fd));
}

static void
program(struct lexer *lexer, struct codegen *codegen)
{
    int i;
    int init;
    int exported;
    lexer_reset(lexer);
    lexer_getch(lexer);

    preprocessor(lexer, codegen);
    decl0(lexer, codegen);

    fprintf(codegen->fd, "section '.idata' import data readable\n");
    init = FALSE;
    exported = 0;
    for (i = 0; i < codegen->ctx->func; ++i) {
        struct function *fun = &codegen->ctx->funs[i];
        if (fun->external) {
            if (init == FALSE) {
                fprintf(codegen->fd, "    library msvcr, '%s',\\\n            kernel32, 'kernel32.dll'\n", fun->dll);
                fprintf(codegen->fd, "    import msvcr, \\\n");
                init = TRUE;
            }
            if (exported > 0) {
                fprintf(codegen->fd, ",\\\n");
            }
            fprintf(codegen->fd, "           %s, '%s'", fun->name, fun->name);
            ++exported;
        }
    }
}

int
main(int argc, char **argv)
{
    struct lexer *lexer = NULL;
    struct codegen *codegen = NULL;
    

    lexer_init(&lexer, "test.c");
    codegen_init(&codegen, "test.asm");
    program(lexer, codegen);
    lexer_destroy(&lexer);
    codegen_destroy(&codegen);
    system("pause");
    return 0;
}
