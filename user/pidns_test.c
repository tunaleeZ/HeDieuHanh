#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
namespace_worker(int id, int readyfd, int gatefd, int resultfd)
{
  int pid, status, children[3], i, j, reaped, valid = 1;
  char byte = 'R';

  if (cont_enter(id) < 0 || getpid() != 1)
    exit(10);

  pid = fork();
  if (pid < 0)
    exit(11);
  if (pid == 0) {
    if (getpid() != 2)
      exit(16);
    write(readyfd, &byte, 1);
    for (;;)
      pause(1);
  }

  if (read(gatefd, &byte, 1) != 1)
    exit(12);
  if (kill(2) < 0)
    exit(13);
  if (wait(&status) != 2 || status >= 0)
    exit(14);
  if (kill(2) >= 0)
    exit(17);

  // Allocate several live children in one namespace and verify fork/wait
  // keep their IDs distinct and namespace-local.
  for (i = 0; i < 3; i++) {
    pid = fork();
    if (pid < 0)
      exit(18);
    if (pid == 0) {
      if (getpid() < 3 || getpid() > 5)
        exit(19);
      pause(5);
      exit(0);
    }
    children[i] = pid;
    if (pid < 3 || pid > 5)
      valid = 0;
    for (j = 0; j < i; j++)
      if (children[j] == pid)
        valid = 0;
  }
  for (i = 0; i < 3; i++) {
    reaped = wait(&status);
    if (reaped != children[0] && reaped != children[1] &&
        reaped != children[2])
      valid = 0;
    if (status != 0)
      valid = 0;
  }
  if (!valid)
    exit(22);

  byte = 'D';
  if (write(resultfd, &byte, 1) != 1)
    exit(15);
  exit(0);
}

static void
namespace_init_exit(int id)
{
  if (cont_enter(id) < 0 || getpid() != 1)
    exit(20);
  if (fork() < 0)
    exit(21);
  if (getpid() == 1) {
    // Leave a live member behind. kexit() must close the namespace, kill it,
    // and transfer it to global init for reaping.
    pause(1);
    exit(0);
  }
  for (;;)
    pause(1);
}

static int
new_container(void)
{
  int id = cont_create(0);
  if (id < 0 || cont_setroot(id, "/") < 0)
    return -1;
  return id;
}

int
main(void)
{
  int a, b, c, ready[2], gate_a[2], gate_b[2], result[2];
  int pid_a, pid_b, pid_c, status;
  char byte;
  struct container_info info;
  int failures = 0;

  printf("=== T03 PID namespace ===\n");
  printf("T03-04 host getpid: %s\n", getpid() > 0 ? "PASS" : "FAIL");
  if (getpid() <= 0)
    failures++;

  a = new_container();
  b = new_container();
  c = new_container();
  if (a < 0 || b < 0 || c < 0 || pipe(ready) < 0 || pipe(gate_a) < 0 ||
      pipe(gate_b) < 0 || pipe(result) < 0) {
    printf("FAIL: setup\n");
    exit(1);
  }

  pid_a = fork();
  if (pid_a == 0)
    namespace_worker(a, ready[1], gate_a[0], result[1]);
  pid_b = fork();
  if (pid_b == 0)
    namespace_worker(b, ready[1], gate_b[0], result[1]);
  if (pid_a < 0 || pid_b < 0) {
    printf("FAIL: fork namespace leaders\n");
    exit(1);
  }

  if (read(ready[0], &byte, 1) != 1 || read(ready[0], &byte, 1) != 1) {
    printf("FAIL: namespace workers did not start\n");
    exit(1);
  }
  printf("T03-01 distinct host-visible leader PIDs: %s\n",
         pid_a != pid_b ? "PASS" : "FAIL");
  printf("T03-02 two independent namespace PID 1s: PASS\n");
  printf("T03-03/04 allocation/getpid: leaders PID 1, workers PID 2, three unique children each\n");
  if (pid_a == pid_b)
    failures++;

  // Run A's kill while B's same-numbered child is held alive. B then kills
  // its own PID 2; this fails if A's virtual PID lookup crossed namespaces.
  write(gate_a[1], "A", 1);
  if (read(result[0], &byte, 1) != 1 || byte != 'D') {
    printf("T03-05/06 namespace A kill and wait: FAIL\n");
    failures++;
  } else {
    printf("T03-05/06 namespace A kill and wait: PASS\n");
  }
  write(gate_b[1], "B", 1);
  if (read(result[0], &byte, 1) != 1 || byte != 'D') {
    printf("T03-06/08 namespace B isolation and kill: FAIL\n");
    failures++;
  } else {
    printf("T03-06/08 namespace B isolation and kill: PASS\n");
  }
  if (wait(&status) != pid_a || status != 0)
    failures++;
  if (wait(&status) != pid_b || status != 0)
    failures++;

  pid_c = fork();
  if (pid_c == 0)
    namespace_init_exit(c);
  if (pid_c < 0 || wait(&status) != pid_c || status != 0) {
    printf("T03-07 namespace-init exit: FAIL\n");
    failures++;
  } else {
    int tries;
    for (tries = 0; tries < 100; tries++) {
      if (cont_info(c, &info) == 0 && info.proc_count == 0)
        break;
      pause(1);
    }
    if (tries == 100) {
      printf("T03-07 namespace-init exit/reap: FAIL (members=%d)\n",
             info.proc_count);
      failures++;
    } else {
      printf("T03-07 namespace-init exit/reap: PASS\n");
    }
  }

  printf("=== T03 result: %d failure(s) ===\n", failures);
  exit(failures != 0);
}
