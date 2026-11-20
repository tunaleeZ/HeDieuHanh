#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DURATION 200
#define BATCH 5000

struct result {
  int id;
  uint64 iterations;
};

static void
worker(int id, int fd)
{
  struct result r;
  volatile uint64 work = 1;

  if (cont_enter(id) < 0)
    exit(1);

  r.id = id;
  r.iterations = 0;

  uint start = uptime();

  while ((uint)(uptime() - start) < DURATION) {
    for (int i = 0; i < BATCH; i++) {
      work = work * 1664525ULL + 1013904223ULL;
      r.iterations++;
    }
  }

  if (write(fd, &r, sizeof(r)) != sizeof(r))
    exit(1);

  close(fd);
  exit(0);
}

int
main(void)
{
  int a, b, fd[2], status;
  int failures = 0;
  uint64 count_a = 0, count_b = 0;
  struct result r;

  printf("=== CPU CONTAINER ISOLATION TEST ===\n");

  a = cont_create(4);
  b = cont_create(4);

  if (a < 0 || b < 0 ||
      cont_setroot(a, "/") < 0 ||
      cont_setroot(b, "/") < 0 ||
      cont_set_cpu(a, 2, 20) < 0 ||
      cont_set_cpu(b, 10, 20) < 0 ||
      pipe(fd) < 0) {
    printf("FAIL: setup\n");
    exit(1);
  }

  int pa = fork();

  if (pa < 0)
    exit(1);

  if (pa == 0) {
    close(fd[0]);
    worker(a, fd[1]);
  }

  int pb = fork();

  if (pb < 0) {
    close(fd[0]);
    close(fd[1]);
    wait(0);
    exit(1);
  }

  if (pb == 0) {
    close(fd[0]);
    worker(b, fd[1]);
  }

  close(fd[1]);

  for (int i = 0; i < 2; i++) {
    if (read(fd[0], &r, sizeof(r)) != sizeof(r)) {
      failures++;
      break;
    }

    if (r.id == a)
      count_a = r.iterations;
    else if (r.id == b)
      count_b = r.iterations;
    else
      failures++;
  }

  close(fd[0]);

  for (int i = 0; i < 2; i++) {
    if (wait(&status) < 0 || status != 0)
      failures++;
  }

  printf("ISOLATION_RESULT: A quota=2 iterations=%ld\n",
         (long)count_a);

  printf("ISOLATION_RESULT: B quota=10 iterations=%ld\n",
         (long)count_b);

  if (count_a > 0 && count_b > count_a)
    printf("PASS: higher-quota worker made more progress\n");
  else {
    printf("FAIL: unexpected workload progress\n");
    failures++;
  }

  struct container_info ia, ib;

  if (cont_info(a, &ia) == 0 &&
      cont_info(b, &ib) == 0 &&
      ia.cpu_quota_ticks == 2 &&
      ib.cpu_quota_ticks == 10 &&
      ia.proc_count == 0 &&
      ib.proc_count == 0 &&
      ia.pages_used == 0 &&
      ib.pages_used == 0) {
    printf("PASS: independent quotas and resource recovery\n");
  } else {
    printf("FAIL: quota or resource recovery\n");
    failures++;
  }

  printf("=== CPU ISOLATION TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
