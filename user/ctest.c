
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
print_info(int id)
{
  struct container_info info;

  if (cont_info(id, &info) < 0) {
    printf("cont_info(%d) failed\n", id);
    return;
  }

  printf("Container %d: count=%d, max=%d, active=%d, max_pages=%d, pages_used=%d\n",
         info.id, info.proc_count, info.max_proc, info.active,
         info.max_pages, info.pages_used);
}

int
main(void)
{
  int id;
  int pid;
  int status;
  int i;
  int failures = 0;

  printf("\n=== Container Test ===\n");

  // Test 1: Check host container.
  printf("\n[Test 1] Host container\n");
  print_info(0);

  // Test 2: Create a container with a limit of 3 processes.
  printf("\n[Test 2] Create container\n");
  id = cont_create(3);

  if (id < 0) {
    printf("cont_create failed\n");
    exit(1);
  }

  printf("Created container ID: %d\n", id);
  print_info(id);

  // Test memory quota configuration and cont_info readback from host.
  printf("[Test 2b] Memory quota configuration\n");
  if (cont_set_mem(id, 64) == 0) {
    struct container_info info;
    if (cont_info(id, &info) == 0 && info.max_pages == 64 && info.pages_used == 0)
      printf("PASS: max_pages read back through cont_info\n");
    else {
      printf("FAIL: memory quota readback mismatch\n");
      failures++;
    }
  } else {
    printf("FAIL: set memory quota\n");
    failures++;
  }
  if (cont_set_mem(id, -1) < 0)
    printf("PASS: negative memory quota rejected\n");
  else {
    printf("FAIL: negative memory quota accepted\n");
    failures++;
  }

  // Test 3: Reject invalid maximum process count.
  printf("\n[Test 3] Invalid container limit\n");
  if (cont_create(-1) < 0)
    printf("PASS: negative limit rejected\n");
  else
    printf("FAIL: negative limit accepted\n");

  // Test 4: Enter the new container.
  printf("\n[Test 4] Enter container\n");
  if (cont_enter(id) < 0) {
    printf("cont_enter failed\n");
    exit(1);
  }

  print_info(id);

  if (cont_set_mem(id, 32) < 0)
    printf("PASS: container cannot configure memory quota\n");
  else {
    printf("FAIL: container configured memory quota\n");
    failures++;
  }

  // Test 5: Fork until the container quota is reached.
  // The current process occupies 1 slot.
  printf("\n[Test 5] Process quota\n");

  for (i = 0; i < 3; i++) {
    pid = fork();

    if (pid < 0) {
      printf("fork returned -1 at attempt %d\n", i + 1);
      break;
    }

    if (pid == 0) {
      // Child stays alive until parent finishes testing.
      pause(100);
      exit(0);
    }

    printf("Created child PID %d\n", pid);
  }

  print_info(id);

  // Test 6: Reap children and verify quota is released.
  printf("\n[Test 6] Reap children\n");

  while (wait(&status) >= 0)
    printf("Child reaped\n");

  print_info(id);

  // Test 7: Fork again after quota is released.
  printf("\n[Test 7] Fork after quota release\n");

  pid = fork();

  if (pid < 0) {
    printf("FAIL: fork still blocked\n");
  } else if (pid == 0) {
    exit(0);
  } else {
    wait(&status);
    printf("PASS: fork succeeded after quota release\n");
  }

  print_info(id);

  printf("\n=== Container Test Finished: %d failures ===\n", failures);
  exit(failures);
}
