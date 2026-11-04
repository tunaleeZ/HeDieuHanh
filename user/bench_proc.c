
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

#define TEST_FORKS 100

int
main(void)
{
  struct sysinfo before, after;
  uint start, elapsed;
  int success = 0;
  int failed = 0;
  int status;

  printf("=== PROCESS BENCHMARK ===\n");

  if (sysinfo(&before) < 0) {
    printf("FAIL: initial sysinfo\n");
    exit(1);
  }

  start = uptime();

  for (int i = 0; i < TEST_FORKS; i++) {
    int pid = fork();

    if (pid < 0) {
      failed++;
      continue;
    }

    if (pid == 0) {
      exit(0);
    }

    if (wait(&status) < 0 || status != 0) {
      printf("FAIL: wait or child exit\n");
      exit(1);
    }

    success++;
  }

  elapsed = uptime() - start;

  if (sysinfo(&after) < 0) {
    printf("FAIL: final sysinfo\n");
    exit(1);
  }

  printf("Fork attempts: %d\n", TEST_FORKS);
  printf("Successful forks: %d\n", success);
  printf("Failed forks: %d\n", failed);

  printf("Elapsed ticks: %d\n", elapsed);

  printf("Process slots before: %ld\n", before.nproc);
  printf("Process slots after: %ld\n", after.nproc);

  printf("Free memory before: %ld\n", before.freemem);
  printf("Free memory after: %ld\n", after.freemem);

  if (success == TEST_FORKS &&
      before.nproc == after.nproc &&
      before.freemem == after.freemem) {
    printf("PASS: process benchmark\n");
  } else {
    printf("CHECK: process benchmark\n");
  }

  exit(0);
}
