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

#if defined(_M_IX86)
typedef u32                uptr;
#elif defined(_M_X64)
typedef u64                uptr;
#else
#error "Unsupported architecture"
#endif


#define MEM_SIZE           4096
#define DATA_SIZE          1024
#define CALL_SIZE          1024
#define ENTRY_SIZE         256
#define STRING_TAB_SIZE    256

struct Entry {
    char *         name;
    char *         text;
    u32            uptr;
    u8             opcod;
    struct Entry  *next;
};

struct Dict {
    struct Entry e[ENTRY_SIZE];
};

struct Str {
    char          *string[STRING_TAB_SIZE];
    u32            id;
};

#pragma push(pack, 1)
struct Data {
    u32  flags;
    uptr paylod;
};
#pragma pop()
#define DATA_FLAG_INTEGER  0x01
#define DATA_FLAG_STRING   0x02

struct Fm {
    u8          memo[MEM_SIZE];
    u32         ip;
    u32         ic;
    struct Data data[DATA_SIZE];
    u32         pdata;
    uptr        call[CALL_SIZE];
    u32         pcall;

    struct Dict dict;
    struct Str  table;
};

enum Instruct {
    Nop = 0x00,
    Ret = 0x01,
    Call = 0x02,
    Add = 0x03,
    Sub = 0x04,
    Mul = 0x05,
    Div = 0x06,
    Push = 0x10,
    Drop = 0x11,
    Dup = 0x12,
    Dot = 0x13,
    PutString = 0x14 /** PUT_STRING (len: 4) (CHAR: len) */
};

static int
dict_hash(const char * const text) {
    int n = strlen(text) - 1;
    int ret = 0;
    while (n >= 0)
        ret += text[n--];
    return ret;
}

static void
dict_add(struct Dict * dict, const char * const name, u32 address, u8 opcod) {
	const int hash = dict_hash(name);
	const int id = hash & (ENTRY_SIZE - 1);
    struct Entry *it = &dict->e[id];
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
        it->next = (struct Entry *)calloc(1, sizeof(struct Entry));
        it = it->next;
    }
    it->name = _strdup(name);
    it->uptr = address;
    it->opcod = opcod;
}

static struct Entry *
dict_find(struct Dict *dict, const char * const name) {
	const int hash = dict_hash(name);
	const int id = hash & (ENTRY_SIZE - 1);
    struct Entry *it = &dict->e[id];
    while (it && it->name) {
        if (!strcmp(it->name, name))
            return it;
    }
    return 0;
}

static __inline struct Data *
data_peek(struct Fm *vm) {
    return &vm->data[vm->pdata];
}

static __inline struct Data *
data_pop(struct Fm *vm) {
    return &vm->data[--vm->pdata];
}

static __inline void
data_push_integer(struct Fm *vm, u32 value) {
    vm->data[vm->pdata].flags = 0 | DATA_FLAG_INTEGER;
    vm->data[vm->pdata].paylod = value;
    ++vm->pdata;
}

static __inline u32
data_pop_integer(struct Fm *vm) {
	const u32 value = vm->data[--vm->pdata].paylod;
    vm->data[vm->pdata].flags = 0;
    return value;
}

static __inline void
data_push_string(struct Fm *vm, char * const string) {
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
fm_run(struct Fm *vm, const unsigned int circles) {
    u32 it = 0;
    u8  is_running = 1;
    while (is_running) {
        switch (vm->memo[vm->ip]) {
        case Add: {
	        const u32 n1 = data_pop_integer(vm);
	        const u32 n2 = data_pop_integer(vm);
            data_push_integer(vm, n1 + n2);
            vm->ip++;
            break;
        }
        case Sub: {
	        const u32 n1 = data_pop_integer(vm);
	        const u32 n2 = data_pop_integer(vm);
            data_push_integer(vm, n1 - n2);
            vm->ip++;
            break;
        }
        case Mul: {
	        const u32 n1 = data_pop_integer(vm);
	        const u32 n2 = data_pop_integer(vm);
            data_push_integer(vm, n1 * n2);
            vm->ip++;
            break;
        }
        case Div: {
	        const u32 n1 = data_pop_integer(vm);
	        const u32 n2 = data_pop_integer(vm);
            data_push_integer(vm, n1 / n2);
            vm->ip++;
            break;
        }
        case Push: {
            ++vm->ip;
	        const u32 n = *((u32 *)&vm->memo[vm->ip]);
            vm->ip += 4;
            data_push_integer(vm, n);
            break;
        }
        case Dup: {
	        const struct Data d = vm->data[vm->pdata];
            ++vm->ip;
            vm->data[vm->pdata++] = d;
            break;
        }
        case Drop: {
            ++vm->ip;
            --vm->pdata;
            break;
        }
        case Dot: {
            struct Data *d = data_pop(vm);
            ++vm->ip;
            if (d->flags & DATA_FLAG_INTEGER) {
                fprintf(stdout, "%d", d->paylod);
            } else {
                fprintf(stdout, "%s", vm->table.string[d->paylod]);
            }
            break;
        }
        case Call: {
            ++vm->ip;
	        const u32 address = *((u32 *)&vm->memo[vm->ip]);
            vm->ip += 4;
            vm->call[vm->pcall++] = vm->ip;
            vm->ip = address;
            break;
        }
        case PutString: {
            u32 d;
            u32 len;
            char *text;

            ++vm->ip;
            len = *((u32 *)&vm->memo[vm->ip]);
            vm->ip += 4;
            text = (char *)calloc(1, len + 1);
            for (d = 0; d < len; d++) {
                text[d] = vm->memo[vm->ip++];
            }
            data_push_string(vm, text);
            free(text);
            break;
        }
        case Ret: {
	        const u32 address = vm->call[--vm->pcall];
            vm->ip++;
            vm->ip = address;
            if (address == 0xffffffff) {
                is_running = 0;
            }
            break;
        }
        }
        ++it;

        if (it >= circles) {
            is_running = 0;
        }
    }
}

static void
fm_default(struct Fm *vm) {
    dict_add(&vm->dict, "+", 0, Add);
    dict_add(&vm->dict, "-", 0, Sub);
    dict_add(&vm->dict, "/", 0, Div);
    dict_add(&vm->dict, "*", 0, Mul);
    dict_add(&vm->dict, "DROP", 0, Drop);
    dict_add(&vm->dict, "DUP", 0, Dup);
    dict_add(&vm->dict, ".", 0, Dot);
    dict_add(&vm->dict, ".\"", 0, PutString);
}

struct Fm *
fm_create() {
    struct Fm *vm = (struct Fm *)calloc(1, sizeof(struct Fm));
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

    while (*p && isspace((int)*p)) ++p;
    while (*p && !isspace((int)*p))
        token[d++] = *p++;
    token[d] = 0;
    return p;
}

static __inline char *
tok_next_char(char *p, char *token, const char ch) {
    int d = 0;

    while (*p && *p != ch)
        token[d++] = *p++;
    token[d] = 0;
    return p;
}

static __inline int
tok_isnumber(char *token) {
	const u32 n = strlen(token);
    u32 i = 0;

    if (!isdigit((int)token[0]))
        return 0;
    while (i < n) {
        if (!isdigit((int)token[i]) && token[i] != '.')
            return 0;
        --i;
    }
    return 1;
}

static __inline int
tok_isstring(const char *token) {
    return token[0] == '.' && token[1] == '"' && token[2] == 0;
}

void
fm_compile(struct Fm *vm, const char * const text) {
    char *p = (char *)text;
    char token[256];

    while (*p != 0) {
        p = tok_next(p, token);
        if (token[0] == 0)
            continue;
        if (token[0] == ':' && token[1] == 0) {
            char name[sizeof(token)];
	        const u32  address = vm->ic;
            p = tok_next(p, token);
            strcpy(name, token);
            while (1) {
                p = tok_next(p, token);
                if (token[0] == ';' && token[1] == 0) {
                    vm->memo[vm->ic++] = Ret;
                    dict_add(&vm->dict, name, address, Nop);
                    break;
                } else if (token[0] == '(' && token[1] == 0) {
                    /** Комментарий */
                    p = tok_next(p, token);
                    while (*p && !(token[0] == ')' && token[1] == 0)) {
                        p = tok_next(p, token);
                    }
                } else if (tok_isnumber(token)) {
	                const u32 n = strtol(token, 0, 10);
                    vm->memo[vm->ic++] = Push;
                    *((u32 *)&vm->memo[vm->ic]) = n;
                    vm->ic += sizeof(u32);
                } else if (tok_isstring(token)) {
                    u32 len;
                    u32 d;
                    /** Строка */
                    ++p;
                    p = tok_next_char(p, token, '"');
                    if (*p != 0)
                        ++p;
                    len = strlen(token);
                    vm->memo[vm->ic++] = PutString;
                    *((u32 *)&vm->memo[vm->ic]) = len;
                    vm->ic += sizeof(u32);
                    for (d = 0; d < strlen(token); d++) {
                        vm->memo[vm->ic++] = token[d];
                    }
                } else {
                    struct Entry *e = dict_find(&vm->dict, token);
                    if (e == 0) {
                        fprintf(stderr, "Function %s not found\n", token);
                        return;
                    }
                    if (e->opcod > Nop) {
                        vm->memo[vm->ic++] = e->opcod;
                    } else {
                        vm->memo[vm->ic++] = Call;
                        *((u32 *)&vm->memo[vm->ic]) = e->uptr;
                        vm->ic += sizeof(u32);
                    }
                }
            }
        } else {
            struct Entry *e = dict_find(&vm->dict, token);
            if (e == 0) {
                if (tok_isnumber(token)) {
                    vm->data[vm->pdata++].paylod = strtol(token, 0, 10);
                } else {
                    fprintf(stderr, "Function %s not found\n", token);
                    return;
                }
            }
            if (e->opcod > Nop) {
	            const u32 ret = vm->ip;
                vm->ip = MEM_SIZE - 1;
                vm->memo[MEM_SIZE - 1] = e->opcod;
                fm_run(vm, 1);
                vm->ip = ret;
            } else {
	            const u32 ret = vm->ip;
                vm->call[vm->pcall++] = 0xffffffff;
                vm->ip = e->uptr;
                fm_run(vm, -1);
                vm->ip = ret;
            }
        }
    }
}

