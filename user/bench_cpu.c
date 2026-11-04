#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  volatile uint64 result = 0;
  uint64 iterations = 0;

  uint start = uptime();
  uint duration = 100;

  printf("=== CPU BENCHMARK ===\n");

  while ((uint)(uptime() - start) < duration) {
    result = result * 1664525ULL + 1013904223ULL;
    iterations++;
  }

  uint elapsed = uptime() - start;

  printf("Elapsed ticks: %d\n", elapsed);
  printf("Iterations: %ld\n", iterations);
  printf("Iterations/tick: %ld\n",
         elapsed ? iterations / elapsed : 0);

  exit(0);
}
