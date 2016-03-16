#include <string.h>
#include <ctype.h>
#include "vm.h"



void
vm_init(struct vm *vm)
{
    memset(vm->memo, 0, sizeof(vm->memo));
    memset(vm->data, 0, sizeof(vm->data));
    memset(vm->call, 0, sizeof(vm->call));
    vm->pdata = 0;
    vm->pcall = 0;
    vm->pmemo = 0;
}

void
vm_compile(const char * const text, struct vm *vm)
{
#define skip_whitespace(ptr, i) \
            while (ptr[++i] == ' ')
#define next_word(ptr, i) \
            while (isalpha((int)ptr[i]) && isalnum((int)ptr[i])) ++i

    u32   i;
    u32   len = strlen(text);
    char *p = text;

    for (i = 0; i < len && p[i] != 0; i++) {
        skip_whitespace(p, i);
        if (p[i] == ':') {
            u32 start;
            skip_whitespace(p, i);
            start = i;
            next_word(p, i);
            //Name
            
            while (p[i] != ';') {
                skip_whitespace(p, i);
                start = i;
                next_word(p, i);
                __asm nop;
            }            
        }
    }
}

