#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("FAIL: outside executable ran\n");
  exit(42);
}
