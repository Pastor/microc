#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#include "vm.h"

#define SIZE    1024

/**
https://defuse.ca/online-x86-assembler.htm
*/

void * 
alloc_memory(size_t size) 
{
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void 
emit_code(uint8_t * m) 
{
    uint8_t *p = m + 25;
    uint8_t call[] = { ((uint8_t *)&p)[0], ((uint8_t *)&p)[1], ((uint8_t *)&p)[2], ((uint8_t *)&p)[3] };
    uint8_t code[] = {
        0x55,              // push ebp
        0x89, 0xe5,        // mov  ebp, esp
        0x8b, 0x45, 0x08,  // mov  eax, dword ptr[ebp + 0x08]
        0x83, 0xc0, 0x04,  // add  eax, 0x04
        0xbb, call[0], call[1], call[2], call[3], //mov ebx, 0x00000000
        0x50,              // push eax
        0xff, 0xd3,        // call ebx

        0x31, 0xdb,        // xor  ebx, ebx
        0x53,              // push ebx
        0x5b,              // pop  ebx

        0x5d,              // pop  ebp
        0xc2, 0x04, 0x00,  // ret  0x04
        
        0x55,              // push ebp
        0x89, 0xe5,        // mov  ebp, esp
        0x8b, 0x45, 0x08,  // mov  eax, dword ptr[ebp + 0x08]
        0x83, 0xc0, 0x06,  // add  eax, 0x06
        0x5d,              // pop  ebp
        0xc2, 0x04, 0x00   // ret  0x04
    };
    memcpy(m, code, sizeof(code));
}

typedef long (__stdcall * JittedCall )( long arg );

void 
start() 
{
    void* m = alloc_memory(SIZE);
    emit_code(m);

    JittedCall call = m;
    int result = call(2);
    printf("result = %d\n", result);
}

int
main(int argc, char **argv)
{
    //start();
    struct vm vm;

    vm_init(&vm);
    vm_compile(": SUM 10 60 + ;", &vm);

    system("pause");
    return EXIT_SUCCESS;
}

