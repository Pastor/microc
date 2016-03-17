#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "fm.h"

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef u32                uptr;


#define MEM_SIZE           4096
#define DATA_SIZE          1024
#define CALL_SIZE          1024
#define ENTRY_SIZE         256
#define STRING_TAB_SIZE    256

struct entry {
    char *         name;
    char *         text;
    u32            uptr;
    u8             opcod;
    struct entry  *next;
};

struct dict {
    struct entry e[ENTRY_SIZE];
};

struct str {
    char          *string[STRING_TAB_SIZE];
    u32            id;
};

#pragma push(pack, 1)
struct data {
    u32  flags;
    u32  paylod;
};
#pragma pop()
#define DATA_FLAG_INTEGER  0x01
#define DATA_FLAG_STRING   0x02

struct fm {
    u8          memo[MEM_SIZE];
    u32         ip;
    u32         ic;
    struct data data[DATA_SIZE];
    u32         pdata;
    uptr        call[CALL_SIZE];
    u32         pcall;

    struct dict dict;
    struct str  table;
};

enum instruct {
    NOP = 0x00,
    RET = 0x01,
    CALL = 0x02,
    ADD = 0x03,
    SUB = 0x04,
    MUL = 0x05,
    DIV = 0x06,
    PUSH = 0x10,
    DROP = 0x11,
    DUP = 0x12,
    DOT = 0x13,
    PUT_STRING = 0x14 /** PUT_STRING (len: 4) (CHAR: len) */
};

static int
dict_hash(const char * const text) {
    int n = strlen(text) - 1;
    int ret = 0;
    while ( n >= 0 )
        ret += text[n--];
    return ret;
}

static void
dict_add(struct dict * dict, const char * const name, u32 address, u8 opcod) {
    int hash = dict_hash(name);
    int id = hash & (ENTRY_SIZE - 1);
    struct entry *it = &dict->e[id];
    if ( it != 0 && it->name != 0 ) {
        while ( it ) {
            if ( !strcmp(it->name, name) ) {
                it->uptr = address;
                it->opcod = opcod;
                return;
            }
            if ( it->next == 0 )
                break;
            it = it->next;
        }
        it->next = (struct entry *)calloc(1, sizeof(struct entry));
        it = it->next;
    }
    it->name = _strdup(name);
    it->uptr = address;
    it->opcod = opcod;
}

static struct entry *
dict_find(struct dict *dict, const char * const name) {
    int hash = dict_hash(name);
    int id = hash & (ENTRY_SIZE - 1);
    struct entry *it = &dict->e[id];
    while ( it && it->name ) {
        if ( !strcmp(it->name, name) )
            return it;
    }
    return 0;
}

static __inline struct data *
data_peek(struct fm *vm)
{
    return &vm->data[vm->pdata];
}

static __inline struct data *
data_pop(struct fm *vm)
{
    return &vm->data[--vm->pdata];
}

static __inline void
data_push_integer(struct fm *vm, u32 value)
{
    vm->data[vm->pdata].flags = 0 | DATA_FLAG_INTEGER;
    vm->data[vm->pdata].paylod = value;
    ++vm->pdata;
}

static __inline u32
data_pop_integer(struct fm *vm)
{
    u32 value = vm->data[--vm->pdata].paylod;
    vm->data[vm->pdata].flags = 0;
    return value;
}

static __inline void
data_push_string(struct fm *vm, char * const string)
{
    u32 i;
    int index = -1;

    for (i = 0; i < STRING_TAB_SIZE; i++) {
        if (vm->table.string[i] == 0)
            continue;
        if (!strcmp(vm->table.string[i], string)) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        index = vm->table.id++;
        vm->table.string[index] = _strdup(string);
    }
    vm->data[vm->pdata].flags = 0 | DATA_FLAG_STRING;
    vm->data[vm->pdata].paylod = (u32)index;
    ++vm->pdata;
}

void
fm_run(struct fm *vm, unsigned int circles) {
    u32 it = 0;
    u8  is_running = 1;
    while ( is_running ) {
        switch ( vm->memo[vm->ip] ) {
            case ADD:
            {
                u32 n1 = data_pop_integer(vm);
                u32 n2 = data_pop_integer(vm);
                data_push_integer(vm, n1 + n2);
                vm->ip++;
                break;
            }
            case SUB:
            {
                u32 n1 = data_pop_integer(vm);
                u32 n2 = data_pop_integer(vm);
                data_push_integer(vm, n1 - n2);
                vm->ip++;
                break;
            }
            case MUL:
            {
                u32 n1 = data_pop_integer(vm);
                u32 n2 = data_pop_integer(vm);
                data_push_integer(vm, n1 * n2);
                vm->ip++;
                break;
            }
            case DIV:
            {
                u32 n1 = data_pop_integer(vm);
                u32 n2 = data_pop_integer(vm);
                data_push_integer(vm, n1 / n2);
                vm->ip++;
                break;
            }
            case PUSH:
            {
                ++vm->ip;
                u32 n = *((u32 *)&vm->memo[vm->ip]);
                vm->ip += 4;
                data_push_integer(vm, n);
                break;
            }
            case DUP:
            {
                struct data d = vm->data[vm->pdata];
                ++vm->ip;
                vm->data[vm->pdata++] = d;
                break;
            }
            case DROP:
            {
                ++vm->ip;
                --vm->pdata;
                break;
            }
            case DOT:
            {
                struct data *d = data_pop(vm);
                ++vm->ip;
                if (d->flags & DATA_FLAG_INTEGER) {
                    fprintf(stdout, "%d", d->paylod);
                } else {
                    fprintf(stdout, "%s", vm->table.string[d->paylod]);
                }
                break;
            }
            case CALL:
            {
                ++vm->ip;
                u32 address = *((u32 *)&vm->memo[vm->ip]);
                vm->ip += 4;
                vm->call[vm->pcall++] = vm->ip;
                vm->ip = address;
                break;
            }
            case PUT_STRING:
            {
                u32 d;
                u32 len;
                char *text;

                ++vm->ip;
                len = *((u32 *)&vm->memo[vm->ip]);
                vm->ip += 4;
                text = (char *)calloc(1, len + 1);
                for ( d = 0; d < len; d++ ) {
                    text[d] = vm->memo[vm->ip++];
                }
                data_push_string(vm, text);
                free(text);
                break;
            }
            case RET:
            {
                u32 address = vm->call[--vm->pcall];
                vm->ip++;
                vm->ip = address;
                if ( address == 0xffffffff ) {
                    is_running = 0;
                }
                break;
            }
        }
        ++it;

        if ( it >= circles ) {
            is_running = 0;
        }
    }
}

static void
fm_default(struct fm *vm) {
    dict_add(&vm->dict, "+", 0, ADD);
    dict_add(&vm->dict, "-", 0, SUB);
    dict_add(&vm->dict, "/", 0, DIV);
    dict_add(&vm->dict, "*", 0, MUL);
    dict_add(&vm->dict, "DROP", 0, DROP);
    dict_add(&vm->dict, "DUP", 0, DUP);
    dict_add(&vm->dict, ".", 0, DOT);
    dict_add(&vm->dict, ".\"", 0, PUT_STRING);
}

struct fm *
fm_create() {
    struct fm *vm = (struct fm *)calloc(1, sizeof(struct fm));
    memset(vm->memo, 0, sizeof(vm->memo));
    memset(vm->data, 0, sizeof(vm->data));
    memset(vm->call, 0, sizeof(vm->call));
    memset(&vm->dict, 0, sizeof(vm->dict));
    vm->pdata = 0;
    vm->pcall = 0;
    vm->ip = 0;
    vm->ic = 0;
    fm_default(vm);
    return vm;
}

static __inline char *
tok_next(char *p, char *token) {
    int d = 0;

    while ( *p && isspace((int)*p) ) ++p;
    while ( *p && !isspace((int)*p) )
        token[d++] = *p++;
    token[d] = 0;
    return p;
}

static __inline char *
tok_next_char(char *p, char *token, char ch) {
    int d = 0;

    while ( *p && *p != ch )
        token[d++] = *p++;
    token[d] = 0;
    return p;
}

static __inline int
tok_isnumber(char *token) {
    u32 n = strlen(token);
    u32 i = 0;
    
    if ( !isdigit((int)token[0]) )
        return 0;
    while ( i < n ) {
        if ( !isdigit((int)token[i]) && token[i] != '.' )
            return 0;
        --i;
    }
    return 1;
}

static __inline int
tok_isstring(char *token) {
    return token[0] == '.' && token[1] == '"' && token[2] == 0;
}

void
fm_compile(struct fm *vm, const char * const text) {
    char *p = (char *)text;
    char token[256];

    while ( *p != 0 ) {
        p = tok_next(p, token);
        if (token[0] == 0)
            continue;
        if ( token[0] == ':' && token[1] == 0 ) {
            char name[sizeof(token)];
            u32  address = vm->ic;
            p = tok_next(p, token);
            strcpy(name, token);
            while ( 1 ) {
                p = tok_next(p, token);
                if ( token[0] == ';' && token[1] == 0 ) {
                    vm->memo[vm->ic++] = RET;
                    dict_add(&vm->dict, name, address, NOP);
                    break;
                } else if ( token[0] == '(' && token[1] == 0 ) {
                    /** Комментарий */
                    p = tok_next(p, token);
                    while ( *p && !(token[0] == ')' && token[1] == 0) ) {
                        p = tok_next(p, token);
                    }
                } else if ( tok_isnumber(token) ) {
                    u32 n = strtol(token, 0, 10);
                    vm->memo[vm->ic++] = PUSH;
                    *((u32 *)&vm->memo[vm->ic]) = n;
                    vm->ic += sizeof(u32);
                } else if ( tok_isstring(token) ) {
                    u32 len;
                    u32 d;
                    /** Строка */
                    ++p;
                    p = tok_next_char(p, token, '"');
                    if ( *p != 0 )
                        ++p;
                    len = strlen(token);
                    vm->memo[vm->ic++] = PUT_STRING;
                    *((u32 *)&vm->memo[vm->ic]) = len;
                    vm->ic += sizeof(u32);
                    for ( d = 0; d < strlen(token); d++ ) {
                        vm->memo[vm->ic++] = token[d];
                    }
                } else {
                    struct entry *e = dict_find(&vm->dict, token);
                    if ( e == 0 ) {
                        fprintf(stderr, "Function %s not found\n", token);
                        return;
                    }
                    if ( e->opcod > NOP ) {
                        vm->memo[vm->ic++] = e->opcod;
                    } else {
                        vm->memo[vm->ic++] = CALL;
                        *((u32 *)&vm->memo[vm->ic]) = e->uptr;
                        vm->ic += sizeof(u32);
                    }
                }
            }
        } else {
            struct entry *e = dict_find(&vm->dict, token);
            if ( e == 0 ) {
                if ( tok_isnumber(token) ) {
                    vm->data[vm->pdata++].paylod = strtol(token, 0, 10);
                } else {
                    fprintf(stderr, "Function %s not found\n", token);
                    return;
                }
            }
            if ( e->opcod > NOP ) {
                u32 ret = vm->ip;
                vm->ip = MEM_SIZE - 1;
                vm->memo[MEM_SIZE - 1] = e->opcod;
                fm_run(vm, 1);
                vm->ip = ret;
            } else {
                u32 ret = vm->ip;
                vm->call[vm->pcall++] = 0xffffffff;
                vm->ip = e->uptr;
                fm_run(vm, -1);
                vm->ip = ret;
            }
        }
    }
}

