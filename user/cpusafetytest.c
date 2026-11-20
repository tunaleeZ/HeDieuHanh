#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DURATION 100
#define WORKERS 3
#define BATCH 5000

static void
worker(int fd, int short_burst)
{
  uint64 count = 0;
  volatile uint64 work = 1;
  uint start = uptime();

  while ((uint)(uptime() - start) < DURATION) {
    for (int i = 0; i < BATCH; i++) {
      work = work * 1664525ULL + 1013904223ULL;
      count++;
    }

    if (short_burst)
      pause(1);
  }

  if (write(fd, &count, sizeof(count)) != sizeof(count))
    exit(1);

  close(fd);
  exit(0);
}

static int
run_case(int quota, int short_burst)
{
  int id, fd[2], pid, status;
  int workers = 0;
  int failures = 0;
  uint64 total = 0;
  uint64 count;
  struct container_info info;

  id = cont_create(8);

  if (id < 0 ||
      cont_setroot(id, "/") < 0 ||
      cont_set_cpu(id, quota, 20) < 0) {
    printf("FAIL: container setup\n");
    return 1;
  }

  if (pipe(fd) < 0)
    return 1;

  pid = fork();

  if (pid < 0)
    return 1;

  if (pid == 0) {
    close(fd[0]);

    if (cont_enter(id) < 0)
      exit(1);

    for (int i = 0; i < WORKERS; i++) {
      int child = fork();

      if (child < 0)
        break;

      if (child == 0)
        worker(fd[1], short_burst);

      workers++;
    }

    close(fd[1]);

    for (int i = 0; i < workers; i++) {
      if (wait(&status) < 0 || status != 0)
        failures++;
    }

    if (workers != WORKERS)
      failures++;

    exit(failures ? 1 : 0);
  }

  close(fd[1]);

  uint start = uptime();

  while ((uint)(uptime() - start) < DURATION) {
    if (cont_info(id, &info) == 0) {
      printf("CPU_SAMPLE: id=%d quota=%ld used=%ld runtime=%ld proc=%d\n",
             id,
             (long)info.cpu_quota_ticks,
             (long)info.cpu_used_ticks,
             (long)info.cpu_used_runtime,
             info.proc_count);
    }

    pause(10);
  }

  int received = 0;

  while (read(fd[0], &count, sizeof(count)) == sizeof(count)) {
    total += count;
    received++;
  }

  close(fd[0]);

  if (wait(&status) != pid || status != 0)
    failures++;

  printf("CPU_RESULT: quota=%d burst=%d workers=%d iterations=%ld\n",
         quota, short_burst, received, (long)total);

  if (received != WORKERS || total == 0)
    failures++;

  if (cont_info(id, &info) < 0 ||
      info.proc_count != 0 ||
      info.pages_used != 0)
    failures++;

  if (failures)
    printf("CPU_CASE: FAIL\n");
  else
    printf("CPU_CASE: PASS\n");

  return failures;
}

int
main(void)
{
  int failures = 0;

  printf("=== SCRUM-95 CPU SAFETY TEST ===\n");

  printf("\n[CPU-S01] Short-burst workload\n");
  failures += run_case(5, 1);

  printf("\n[CPU-S02] Multi-worker shared quota\n");
  failures += run_case(5, 0);

  printf("\n[CPU-S02] Unlimited CPU reference\n");
  failures += run_case(0, 0);

  printf("\n=== CPU SAFETY TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
