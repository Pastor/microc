#include <stdio.h>
#include <stdlib.h>
#include "fm.h"


int
main(int argc, char **argv)
{
    struct fm *fm = fm_create();
    fm_compile(fm, ": SUM 10 60 + ; SUM");
    system("pause");
    return EXIT_SUCCESS;
}

