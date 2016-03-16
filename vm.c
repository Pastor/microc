#include <string.h>
#include <stdlib.h>
#include "vm.h"

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
    struct entry  *next;
};

struct dict
{
    struct entry e[ENTRY_SIZE];
};

struct vm
{
    u8   memo[MEM_SIZE];
    u32  pmemo;
    uptr data[DATA_SIZE];
    u32  pdata;
    uptr call[CALL_SIZE];
    u32  pcall;

    struct dict dict;
};

enum instruct
{
    RET = 0x00,
    CALL = 0x01,
    ADD = 0x02,
    PUSH = 0x03
};

struct _instruct
{
    u8                 op;
    const char * const name;
} instructions[] = {
    { RET, "RET" },
    { CALL, "CALL" },
    { ADD, "+" },
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
dict_add(struct dict * dict, const char * const name, u32 p)
{
    int hash = dict_hash(name);
    int id = hash & (ENTRY_SIZE - 1);
    struct entry *it = &dict->e[id];
    if (it != 0 && it->name != 0) {
        while (it) {
            if (!strcmp(it->name, name)) {
                it->uptr = p;
                return;
            }
            if (it->next == 0)
                break;
            it = it->next;
        }
        it->next = (struct entry *)calloc(1, sizeof(struct entry));
        it = it->next;
    }
    it->name = strdup(name);
    it->uptr = p;
}

static void
vm_default(struct vm *vm)
{
    u32 p = vm->pmemo;
    vm->memo[vm->pmemo++] = instructions[ADD].op;
    vm->memo[vm->pmemo++] = instructions[RET].op;
    dict_add(&vm->dict, instructions[ADD].name, p);
}

void
vm_run(struct vm *vm, unsigned int circles)
{
    u32 it = 0;
    while (it < circles) {
        switch (vm->memo[vm->pmemo]) {
        case ADD: {
            u32 n1 = vm->data[vm->pdata--];
            u32 n2 = vm->data[vm->pdata--];
            vm->data[++vm->pdata] = n1 + n2;
            vm->pmemo++;
            break;
        }
        case PUSH: {
            u32 n = (u32)vm->memo;
            vm->pmemo++;
            vm->pmemo += 4;
            vm->data[vm->pdata++] = n;            
            break;
        }
        case CALL: {
            u32 address = (u32)vm->memo;
            vm->pmemo++;
            vm->pmemo += 4;
            vm->call[vm->pcall++] = vm->pmemo;
            vm->pmemo = address;
            break;
        }
        case RET: {
            u32 address = vm->call[vm->pcall--];
            vm->pmemo++;
            vm->pmemo = address;
            break;
        }
        }
        ++it;
    }
}

struct vm *
vm_create()
{
    struct vm *vm = (struct vm *)calloc(1, sizeof(struct vm));
    memset(vm->memo, 0, sizeof(vm->memo));
    memset(vm->data, 0, sizeof(vm->data));
    memset(vm->call, 0, sizeof(vm->call));
    memset(&vm->dict, 0, sizeof(vm->dict));
    vm->pdata = 0;
    vm->pcall = 0;
    vm->pmemo = 0;
    vm_default(vm);
    return vm;
}

void
vm_compile(const char * const text, struct vm *vm)
{}

