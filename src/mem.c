#include "mem.h"

#include <stdio.h>
#include <stdlib.h>


_Noreturn void outOfMemory(void)
{
    fprintf(stderr, "casegen: out of memory\n");
    exit(1);
}
