#pragma once

struct fm *
fm_create();

void
fm_compile(struct fm *vm, const char * const text);

void
fm_run(struct fm *vm, unsigned int circles);