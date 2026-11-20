#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PAGE_SIZE 4096
#define LIMIT 32
#define BAD_ADDR ((char *)-1)

static int failures = 0;

static void
check(int condition, char *name)
{
  if (condition) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s\n", name);
    failures++;
  }
}

static int
used_pages(int id)
{
  struct container_info info;

  if (cont_info(id, &info) < 0) {
    printf("FAIL: cont_info(%d)\n", id);
    exit(1);
  }

  return info.pages_used;
}

static void
show_usage(int id)
{
  struct container_info info;

  if (cont_info(id, &info) < 0)
    exit(1);

  printf("Container %d: proc=%d/%d pages=%d/%d\n",
         id, info.proc_count, info.max_proc,
         info.pages_used, info.max_pages);
}

static void
container_tests(int id)
{
  int base, before, pid, status, fill;
  uint64 addr;
  int padding;
  char *mem;
  volatile char *lazy;

  if (cont_enter(id) < 0) {
    printf("FAIL: cont_enter\n");
    exit(1);
  }

  // Align the heap before testing page-sized changes.
  addr = (uint64)sbrk(0);
  padding = (PAGE_SIZE - addr % PAGE_SIZE) % PAGE_SIZE;

  if (padding && sbrk(padding) == BAD_ADDR)
    exit(1);

  base = used_pages(id);

  printf("\n[T07.1] Initial accounting\n");
  check(base > 0 && base < LIMIT / 2,
        "Initial physical pages accounted");
  show_usage(id);

  if (base <= 0 || base >= LIMIT / 2)
    exit(1);

  printf("\n[T07.2] Eager allocation\n");

  mem = sbrk(4 * PAGE_SIZE);

  check(mem != BAD_ADDR,
        "Eager allocation of 4 pages");

  if (mem == BAD_ADDR)
    exit(1);

  for (int i = 0; i < 4; i++)
    mem[i * PAGE_SIZE] = 'A';

  check(used_pages(id) == base + 4,
        "Accounting increased by 4 pages");
  show_usage(id);

  printf("\n[T09.1] Shrink eager memory\n");

  check(sbrk(-4 * PAGE_SIZE) != BAD_ADDR,
        "Heap shrink succeeded");

  check(used_pages(id) == base,
        "Eager pages released");

  printf("\n[T07.3] Lazy allocation\n");

  before = used_pages(id);
  lazy = (volatile char *)sbrklazy(4 * PAGE_SIZE);

  if ((char *)lazy == BAD_ADDR) {
    printf("FAIL: sbrklazy\n");
    exit(1);
  }

  check(used_pages(id) == before,
        "Lazy reservation consumes no physical pages");

  lazy[0] = 'B';
  lazy[PAGE_SIZE] = 'C';

  check(used_pages(id) == before + 2,
        "Only 2 touched pages accounted");
  show_usage(id);

  check(sbrk(-4 * PAGE_SIZE) != BAD_ADDR,
        "Lazy heap shrink succeeded");

  check(used_pages(id) == before,
        "Only allocated lazy pages released");

  printf("\n[T08.1] Allocation above quota\n");

  before = used_pages(id);

  mem = sbrk(LIMIT * PAGE_SIZE);

  check(mem == BAD_ADDR,
        "Allocation above quota rejected");

  if (mem != BAD_ADDR)
    sbrk(-LIMIT * PAGE_SIZE);

  check(used_pages(id) == before,
        "No quota leak after rejected allocation");

  printf("\n[T08.2] Fork above memory quota\n");

  fill = LIMIT - used_pages(id);

  if (fill <= 0 ||
      sbrk(fill * PAGE_SIZE) == BAD_ADDR) {
    printf("FAIL: Cannot fill memory quota\n");
    exit(1);
  }

  check(used_pages(id) == LIMIT,
        "Memory quota fully occupied");

  before = used_pages(id);

  pid = fork();

  if (pid == 0)
    exit(0);

  check(pid == -1,
        "Fork rejected by memory quota");

  if (pid > 0)
    wait(&status);

  check(used_pages(id) == before,
        "No quota leak after rejected fork");

  printf("\n[T08.5] Exec above memory quota\n");

  char *args[] = {"echo", "unexpected", 0};

  int ret = exec("/echo", args);

  check(ret == -1,
        "Exec rejected when no free quota");

  check(used_pages(id) == before,
        "Old image preserved after exec failure");

  check(sbrk(-fill * PAGE_SIZE) != BAD_ADDR,
        "Release filled memory");

  check(used_pages(id) == base,
        "Memory returned to baseline");

  printf("\n[T09.3] Fork and wait recovery\n");

  before = used_pages(id);
  pid = fork();

  if (pid == 0)
    exit(0);

  check(pid > 0,
        "Fork succeeds after quota recovery");

  if (pid > 0) {
    check(wait(&status) == pid && status == 0,
          "Child successfully reaped");
  }

  check(used_pages(id) == before,
        "Child memory fully reclaimed");

  printf("\n[T08.4] Lazy allocation above quota\n");

  before = used_pages(id);
  pid = fork();

  if (pid == 0) {
    volatile char *ptr;

    ptr = (volatile char *)
        sbrklazy(LIMIT * PAGE_SIZE);

    if ((char *)ptr == BAD_ADDR)
      exit(2);

    for (int i = 0; i < LIMIT; i++)
      ptr[i * PAGE_SIZE] = 'X';

    // Reaching here means quota enforcement failed.
    exit(3);
  }

  check(pid > 0,
        "Created lazy allocation test child");

  if (pid > 0) {
    status = 0;

    check(wait(&status) == pid && status == -1,
          "Lazy quota violation terminated child");
  }

  check(used_pages(id) == before,
        "Lazy violation memory reclaimed");

  show_usage(id);

  printf("\nContainer tests: %d failures\n", failures);
  exit(failures ? 1 : 0);
}

int
main(void)
{
  int id, small, pid, status, original_pid;

  printf("\n=== SCRUM-94 MEMORY QUOTA TEST ===\n");

  id = cont_create(8);

  if (id < 0 ||
      cont_setroot(id, "/") < 0 ||
      cont_set_mem(id, LIMIT) < 0) {
    printf("FAIL: Container setup\n");
    exit(1);
  }

  pid = fork();

  if (pid < 0)
    exit(1);

  if (pid == 0)
    container_tests(id);

  status = 0;

  check(wait(&status) == pid && status == 0,
        "Container internal tests completed");

  printf("\n[T09.4] Full container recovery\n");

  check(used_pages(id) == 0,
        "Container pages_used returned to zero");

  struct container_info info;

  check(cont_info(id, &info) == 0 &&
        info.proc_count == 0,
        "Container proc_count returned to zero");

  show_usage(id);

  printf("\n[T08.3] Entry above memory quota\n");

  small = cont_create(8);

  if (small < 0 ||
      cont_setroot(small, "/") < 0 ||
      cont_set_mem(small, 1) < 0) {
    printf("FAIL: Small container setup\n");
    exit(1);
  }

  original_pid = getpid();

  if (cont_enter(small) == 0) {
    printf("FAIL: Entry above quota accepted\n");
    exit(1);
  }

  check(getpid() == original_pid,
        "PID unchanged after rejected entry");

  check(cont_info(small, &info) == 0 &&
        info.pages_used == 0 &&
        info.proc_count == 0,
        "Rejected entry leaves target unchanged");

  printf("\n=== MEMORY QUOTA TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
