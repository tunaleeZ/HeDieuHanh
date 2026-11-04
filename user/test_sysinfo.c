#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(void)
{
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    printf("FAIL: sysinfo syscall\n");
    exit(1);
  }

  printf("=== SYSTEM RESOURCE INFORMATION ===\n");
  printf("Free memory: %ld bytes\n", info.freemem);
  printf("Used process slots: %ld\n", info.nproc);
  printf("PASS: sysinfo syscall\n");

  exit(0);
}
