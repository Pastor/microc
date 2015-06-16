#include <stdlib.h>
#include <stdio.h>

static int var1;
const char * const v56 = "HELLO";
char * var2 = "";
int va3 = 0;

int
test2()
{
    return 123 + 5 * 70;
}

int
main1(int argc, char **argv)
{
    int i;
    int p = 0;
    const char v = 'c';
    void *pp;

    pp = malloc(1000);
    i = 9;
    p = i + 10;
    test2();
    free(pp);
    fclose(0);
    return 0;
}
