#pragma once
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

struct vm {
    u8   memo[MEM_SIZE];
    u32  pmemo;
    uptr data[DATA_SIZE];
    u32  pdata;
    uptr call[CALL_SIZE];
    u32  pcall;
};

void
vm_init(struct vm *vm);

void
vm_compile(const char * const text, struct vm *vm);

