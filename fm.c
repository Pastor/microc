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

struct entry
{
    char *         name;
    char *         text;
    u32            uptr;
    u8             opcod;
    struct entry  *next;
};

struct dict
{
    struct entry e[ENTRY_SIZE];
};

struct fm
{
    u8   memo[MEM_SIZE];
    u32  ip;
    u32  ic;
    uptr data[DATA_SIZE];
    u32  pdata;
    uptr call[CALL_SIZE];
    u32  pcall;

    struct dict dict;
};

enum instruct
{
    NOP = 0x00,
    RET = 0x01,
    CALL = 0x02,
    ADD = 0x03,
    PUSH = 0x04
};

struct _instruct
{
    u8                 op;
    const char * const name;
} instructions[] = {
    { RET, "RET" },
    { CALL, "CALL" },
    { ADD, "ADD" },
    { PUSH, "PUSH" },
};


static int
dict_hash(const char * const text)
{
    int n = strlen(text) - 1;
    int ret = 0;
    while (n >= 0)
        ret += text[n--];
    return ret;
}

static void
dict_add(struct dict * dict, const char * const name, u32 address, u8 opcod)
{
    int hash = dict_hash(name);
    int id = hash & (ENTRY_SIZE - 1);
    struct entry *it = &dict->e[id];
    if (it != 0 && it->name != 0) {
        while (it) {
            if (!strcmp(it->name, name)) {
                it->uptr = address;
                it->opcod = opcod;
                return;
            }
            if (it->next == 0)
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
dict_find(struct dict *dict, const char * const name)
{
    int hash = dict_hash(name);
    int id = hash & (ENTRY_SIZE - 1);
    struct entry *it = &dict->e[id];
    while (it && it->name) {
        if (!strcmp(it->name, name))
            return it;
    }
    return 0;
}

void
fm_run(struct fm *vm, unsigned int circles)
{
    u32 it = 0;
    while (it < circles) {
        switch (vm->memo[vm->ip]) {
        case ADD: {
            u32 n1 = vm->data[--vm->pdata];
            u32 n2 = vm->data[--vm->pdata];
            vm->data[vm->pdata++] = n1 + n2;
            vm->ip++;
            break;
        }
        case PUSH: {
            ++vm->ip;
            u32 n = *((u32 *)&vm->memo[vm->ip]);
            vm->ip += 4;
            vm->data[vm->pdata++] = n;            
            break;
        }
        case CALL: {
            ++vm->ip;
            u32 address = *((u32 *)&vm->memo[vm->ip]);
            vm->ip += 4;
            vm->call[vm->pcall++] = vm->ip;
            vm->ip = address;
            break;
        }
        case RET: {
            u32 address = vm->call[--vm->pcall];
            vm->ip++;
            vm->ip = address;
            if (address == 0xffffffff) {
                it = circles;
            }
            break;
        }
        }
        ++it;
    }
}

static void
fm_default(struct fm *vm)
{
    dict_add(&vm->dict, "+", 0, ADD);
}

struct fm *
fm_create()
{
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
tok_next(char *p, char *token)
{
    int d = 0;

    while (*p && isspace((int)*p)) ++p;
    while (*p && !isspace((int)*p))
        token[d++] = *p++;
    token[d] = 0;
    return p;
}


void
fm_compile(struct fm *vm, const char * const text)
{
    char *p = (char *)text;
    char token[256];

    while (*p != 0) {
        p = tok_next(p, token);
        if (token[0] == ':' && token[1] == 0) {
            char name[sizeof(token)];
            u32  address = vm->ic;
            p = tok_next(p, token);
            strcpy(name, token);
            while (1) {
                p = tok_next(p, token);
                if (token[0] == ';') {
                    vm->memo[vm->ic++] = RET;
                    dict_add(&vm->dict, name, address, NOP);
                    break;
                }
                if (isdigit((int)token[0])) {
                    u32 n = strtol(token, 0, 10);
                    vm->memo[vm->ic++] = PUSH;
                    *((u32 *)&vm->memo[vm->ic]) = n;
                    vm->ic += sizeof(u32);
                } else {
                    struct entry *e = dict_find(&vm->dict, token);
                    if (e == 0) {
                        fprintf(stderr, "Function %s not found\n", token);
                        return;
                    }
                    if (e->opcod > NOP) {
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
            if (e == 0) {
                fprintf(stderr, "Function %s not found\n", token);
                return;
            }
            if (e->opcod > NOP) {
                //??
            } else {
                u32 ret = vm->ip;
                vm->call[vm->pcall++] = 0xffffffff;
                vm->ip = e->uptr;
                fm_run(vm, 1024);
                vm->ip = ret;
            }
        }
    }
}

