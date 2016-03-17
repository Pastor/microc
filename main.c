#include <stdio.h>
#include <stdlib.h>
#include "fm.h"


int
main(int argc, char **argv)
{
    struct fm *fm = fm_create();
    fm_compile(fm, ": SUM ( a b -- c ) 10 60 + .\" HELLO\n\" ; SUM . . ");

    fprintf(stdout, "\n");
    system("pause");
    return EXIT_SUCCESS;
}

