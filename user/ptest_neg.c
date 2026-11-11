#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int failures;

static void
check(char *name, int ok)
{
  if (ok) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s\n", name);
    failures++;
  }
}

int
main(void)
{
  int id, result;
  struct container_info before, after, info;

  printf("=== CONTAINER PERMISSION TEST ===\n");

  check("host chroot rejects regular file", chroot("/echo") < 0);
  check("host chroot rejects missing path", chroot("/khong_ton_tai") < 0);
  check("cont_info rejects negative id", cont_info(-1, &info) < 0);
  check("cont_info rejects out-of-range id",
        cont_info(999, &info) < 0);

  result = cont_info(0, &before);
  check("host cont_info works", result == 0);
  check("host may enter the current host container", cont_enter(0) == 0);
  result = cont_info(0, &after);
  check("same-container enter does not change process count",
        result == 0 && before.proc_count == after.proc_count);

  id = cont_create(8);
  check("host can create a container", id > 0);
  if (id <= 0) {
    printf("=== PERMISSION TEST FINISHED: %d failures ===\n", failures + 1);
    exit(1);
  }

  check("host can enter the new container", cont_enter(id) == 0);

  check("container cannot create a container", cont_create(4) < 0);
  check("container cannot enter host", cont_enter(0) < 0);
  check("container cannot enter another container", cont_enter(id + 1) < 0);
  check("container cannot chroot", chroot("/") < 0);
  check("container cannot set a root", cont_setroot(id, "/") < 0);
  check("container cannot set CPU quota", cont_set_cpu(id, 5, 20) < 0);
  check("container cannot set memory quota", cont_set_mem(id, 10) < 0);

  result = cont_info(id, &info);
  check("cont_info remains available inside container",
        result == 0 && info.id == id && info.active);
  check("cont_info for host remains available inside container",
        cont_info(0, &info) == 0 && info.id == 0);

  printf("=== PERMISSION TEST FINISHED: %d failures ===\n", failures);
  exit(failures == 0 ? 0 : 1);
}
