
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(void)
{
    hello();

    printf("user: hello() returned, back in user mode\n");

    exit(0);
}

