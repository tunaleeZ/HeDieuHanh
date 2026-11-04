#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

#define PAGE_SIZE 4096
#define TEST_PAGES 32

int
main(void)
{
  struct sysinfo before, after, released;
  char *mem;

  printf("=== MEMORY BENCHMARK ===\n");

  if (sysinfo(&before) < 0) {
    printf("FAIL: sysinfo before allocation\n");
    exit(1);
  }

  mem = sbrk(TEST_PAGES * PAGE_SIZE);

  if (mem == (char *)-1) {
    printf("FAIL: sbrk allocation\n");
    exit(1);
  }

  // Touch every page to trigger actual physical allocation.
  for (int i = 0; i < TEST_PAGES; i++)
    mem[i * PAGE_SIZE] = 1;

  if (sysinfo(&after) < 0) {
    printf("FAIL: sysinfo after allocation\n");
    exit(1);
  }

  if (sbrk(-(TEST_PAGES * PAGE_SIZE)) == (char *)-1) {
    printf("FAIL: memory release\n");
    exit(1);
  }

  if (sysinfo(&released) < 0) {
    printf("FAIL: sysinfo after release\n");
    exit(1);
  }

  printf("Free memory before: %ld bytes\n", before.freemem);
  printf("Free memory after: %ld bytes\n", after.freemem);
  printf("Free memory released: %ld bytes\n", released.freemem);

  printf("Memory used: %ld bytes\n",
         (long)(before.freemem - after.freemem));

  printf("Memory recovered: %ld bytes\n",
         (long)(released.freemem - after.freemem));

  exit(0);
}
