#pragma once

struct vm *
vm_create();

void
vm_compile(const char * const text, struct vm *vm);

void
vm_run(struct vm *vm, unsigned int circles);