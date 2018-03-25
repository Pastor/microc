#pragma once

struct Fm *
fm_create();

void
fm_compile(struct Fm *vm, const char * const text);

void
fm_run(struct Fm *vm, unsigned int circles);