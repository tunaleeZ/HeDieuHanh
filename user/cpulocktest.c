#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define ROUNDS 100

static void
file_worker(char tag)
{
  int fd;
  char data[512];
  uint start;

  memset(data, tag, sizeof(data));

  fd = open("/cpulock.data", O_WRONLY);

  if (fd < 0)
    exit(1);

  start = uptime();

  for (int i = 0; i < ROUNDS; i++) {
    if (write(fd, data, sizeof(data)) != sizeof(data)) {
      close(fd);
      exit(1);
    }

    // Periodically yield through filesystem operations.
    close(fd);

    fd = open("/cpulock.data", O_WRONLY);

    if (fd < 0)
      exit(1);
  }

  close(fd);

  printf("LOCK_RESULT: worker=%c rounds=%d elapsed=%d ticks\n",
         tag, ROUNDS, uptime() - start);

  exit(0);
}

int
main(void)
{
  int id, fd, a, b, pid, status;
  int failures = 0;

  printf("=== SCRUM-95 CPU LOCK TEST ===\n");

  fd = open("/cpulock.data", O_CREATE | O_WRONLY);

  if (fd < 0)
    exit(1);

  close(fd);

  id = cont_create(4);

  if (id < 0 ||
      cont_setroot(id, "/") < 0 ||
      cont_set_cpu(id, 1, 20) < 0) {
    printf("FAIL: container setup\n");
    exit(1);
  }

  a = fork();

  if (a < 0)
    exit(1);

  if (a == 0) {
    if (cont_enter(id) < 0)
      exit(1);

    file_worker('A');
  }

  b = fork();

  if (b < 0)
    exit(1);

  if (b == 0)
    file_worker('B');

  for (int i = 0; i < 2; i++) {
    pid = wait(&status);

    if (pid < 0 || status != 0)
      failures++;
    else
      printf("PASS: worker %d completed\n", pid);
  }

  unlink("/cpulock.data");

  printf("=== CPU LOCK TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
