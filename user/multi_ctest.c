#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define PGSIZE 4096
#define DURATION 100

struct result {
  int id;
  int failures;
  uint64 iterations;
};

static int failures;

static void
check(int ok, char *name)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
  if (!ok)
    failures++;
}

static int
create_file(char *path, char value)
{
  int fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0)
    return -1;

  int n = write(fd, &value, 1);
  close(fd);
  return n == 1 ? 0 : -1;
}

static int
blocked(char *path)
{
  int fd = open(path, O_RDONLY);

  if (fd < 0)
    return 1;

  close(fd);
  return 0;
}

static int
read_marker(char expected)
{
  char value = 0;
  int fd = open("/marker", O_RDONLY);

  if (fd < 0)
    return 0;

  int n = read(fd, &value, 1);
  close(fd);

  return n == 1 && value == expected;
}

static void
worker(int id, char tag, int maxproc, int maxpages,
       int quota, int readyfd, int gatefd, int resultfd)
{
  struct container_info info, before, after;
  struct result r;
  int children = 0;
  int status;
  int rejected = 0;
  char token;

  failures = 0;

  if (cont_enter(id) < 0) {
    printf("FAIL: container entry %c\n", tag);
    exit(1);
  }

  printf("CONTAINER_READY: %c id=%d pid=%d\n",
         tag, id, getpid());

  check(getpid() == 1, "namespace leader PID 1");

  check(read_marker(tag), "correct private root marker");

  check(blocked(tag == 'A' ? "/B.txt" : "/A.txt"),
        "other container private file blocked");

  check(blocked("/outside14"), "host file blocked");

  check(blocked("/../../outside14"),
        "parent traversal blocked");

  check(blocked(tag == 'A' ?
                "/../t14b/B.txt" :
                "/../t14a/A.txt"),
        "cross-root traversal blocked");

  check(chdir("..") == 0 && read_marker(tag),
        "cwd remains inside root");

  if (write(readyfd, &tag, 1) != 1)
    exit(1);

  close(readyfd);

  if (read(gatefd, &token, 1) != 1)
    exit(1);

  close(gatefd);

  check(cont_info(id, &info) == 0 &&
        info.max_proc == maxproc &&
        info.max_pages == maxpages &&
        info.cpu_quota_ticks == quota &&
        info.cpu_period_ticks == 20,
        "independent quota configuration");

  // Memory allocation and recovery.
  if (cont_info(id, &before) < 0)
    exit(1);

  char *mem = sbrk(2 * PGSIZE);

  if (mem != (char *)-1) {
    mem[0] = tag;
    mem[PGSIZE] = tag;

    check(cont_info(id, &after) == 0 &&
          after.pages_used == before.pages_used + 2,
          "memory charged to current container");

    check(sbrk(-2 * PGSIZE) != (char *)-1,
          "memory shrink succeeded");

    check(cont_info(id, &after) == 0 &&
          after.pages_used == before.pages_used,
          "memory accounting recovered");
  } else {
    check(0, "memory allocation below quota");
  }

  mem = sbrk(maxpages * PGSIZE);

  if (mem == (char *)-1) {
    check(1, "allocation above memory quota rejected");
  } else {
    check(0, "allocation above quota accepted");
    sbrk(-maxpages * PGSIZE);
  }

  // Hold children using a pipe until quota is reached.
  int hold[2];

  if (pipe(hold) < 0)
    exit(1);

  for (int i = 0; i < maxproc; i++) {
    int pid = fork();

    if (pid < 0) {
      rejected++;
      break;
    }

    if (pid == 0) {
      close(hold[1]);
      read(hold[0], &token, 1);
      close(hold[0]);
      exit(0);
    }

    children++;
  }

  check(children == maxproc - 1 && rejected > 0,
        "process quota enforced");

  check(cont_info(id, &info) == 0 &&
        info.proc_count == maxproc,
        "container reached its own process limit");

  close(hold[0]);
  close(hold[1]);

  for (int i = 0; i < children; i++) {
    if (wait(&status) < 0 || status != 0)
      failures++;
  }

  check(cont_info(id, &info) == 0 &&
        info.proc_count == 1,
        "child processes reclaimed");

  // CPU workload.
  volatile uint64 work = 1;
  uint64 iterations = 0;
  uint start = uptime();

  while ((uint)(uptime() - start) < DURATION) {
    for (int i = 0; i < 5000; i++) {
      work = work * 1664525ULL + 1013904223ULL;
      iterations++;
    }
  }

  check(iterations > 0, "CPU workload made progress");

  r.id = id;
  r.failures = failures;
  r.iterations = iterations;

  if (write(resultfd, &r, sizeof(r)) != sizeof(r))
    exit(1);

  close(resultfd);

  exit(failures ? 1 : 0);
}

int
main(void)
{
  int a, b, pa, pb, status;
  int ready[2], gate[2], result[2];
  char token;
  struct result r;
  struct container_info ia, ib;
  uint64 counta = 0, countb = 0;

  printf("=== T14 MULTI-CONTAINER TEST ===\n");

  if (mkdir("/t14a") < 0 ||
      mkdir("/t14b") < 0 ||
      create_file("/t14a/marker", 'A') < 0 ||
      create_file("/t14b/marker", 'B') < 0 ||
      create_file("/t14a/A.txt", 'A') < 0 ||
      create_file("/t14b/B.txt", 'B') < 0 ||
      create_file("/outside14", 'H') < 0) {
    printf("FAIL: root fixture setup\n");
    exit(1);
  }

  a = cont_create(4);
  b = cont_create(6);

  if (a < 0 || b < 0 ||
      cont_setroot(a, "/t14a") < 0 ||
      cont_setroot(b, "/t14b") < 0 ||
      cont_set_mem(a, 32) < 0 ||
      cont_set_mem(b, 64) < 0 ||
      cont_set_cpu(a, 2, 20) < 0 ||
      cont_set_cpu(b, 10, 20) < 0) {
    printf("FAIL: container configuration\n");
    exit(1);
  }

  if (pipe(ready) < 0 ||
      pipe(gate) < 0 ||
      pipe(result) < 0)
    exit(1);

  pa = fork();

  if (pa == 0) {
    close(ready[0]);
    close(gate[1]);
    close(result[0]);

    worker(a, 'A', 4, 32, 2,
           ready[1], gate[0], result[1]);
  }

  if (pa < 0)
    exit(1);

  pb = fork();

  if (pb == 0) {
    close(ready[0]);
    close(gate[1]);
    close(result[0]);

    worker(b, 'B', 6, 64, 10,
           ready[1], gate[0], result[1]);
  }

  if (pb < 0) {
    close(gate[1]);
    wait(0);
    exit(1);
  }

  close(ready[1]);
  close(gate[0]);
  close(result[1]);

  int ra = read(ready[0], &token, 1);
  int rb = read(ready[0], &token, 1);

  check(ra == 1 && rb == 1,
        "both containers ready");

  check(cont_info(a, &ia) == 0 &&
        cont_info(b, &ib) == 0 &&
        ia.proc_count == 1 &&
        ib.proc_count == 1,
        "two containers active simultaneously");

  close(ready[0]);

  check(write(gate[1], "XX", 2) == 2,
        "release both workloads");

  close(gate[1]);

  for (int i = 0; i < 15; i++) {
    if (cont_info(a, &ia) == 0 &&
        cont_info(b, &ib) == 0) {

      printf("MULTI_SAMPLE: A proc=%d pages=%d "
             "B proc=%d pages=%d\n",
             ia.proc_count, ia.pages_used,
             ib.proc_count, ib.pages_used);

      if (ia.proc_count > 4 || ib.proc_count > 6 ||
          ia.pages_used > 32 || ib.pages_used > 64)
        failures++;
    } else {
      failures++;
    }

    printf("HOST_PROGRESS: tick=%d\n", uptime());
    pause(10);
  }

  for (int i = 0; i < 2; i++) {
    if (read(result[0], &r, sizeof(r)) != sizeof(r)) {
      failures++;
      break;
    }

    failures += r.failures;

    if (r.id == a)
      counta = r.iterations;
    else if (r.id == b)
      countb = r.iterations;
    else
      failures++;
  }

  close(result[0]);

  for (int i = 0; i < 2; i++) {
    if (wait(&status) < 0 || status != 0)
      failures++;
  }

  printf("MULTI_RESULT: A iterations=%ld\n",
         (long)counta);

  printf("MULTI_RESULT: B iterations=%ld\n",
         (long)countb);

  check(counta > 0 && countb > 0,
        "both CPU workloads completed");

  check(cont_info(a, &ia) == 0 &&
        cont_info(b, &ib) == 0 &&
        ia.proc_count == 0 &&
        ib.proc_count == 0 &&
        ia.pages_used == 0 &&
        ib.pages_used == 0,
        "both containers fully reclaimed");

  check(!blocked("/outside14"),
        "host filesystem remains accessible");

  unlink("/t14a/marker");
  unlink("/t14b/marker");
  unlink("/t14a/A.txt");
  unlink("/t14b/B.txt");
  unlink("/outside14");
  unlink("/t14a");
  unlink("/t14b");

  printf("=== T14 MULTI-CONTAINER TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
