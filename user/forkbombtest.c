#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define LIMIT 8
#define MAX_DEPTH 5

static void
stress(int depth, int fd)
{
  int children = 0;
  int status;
  char event;

  if (depth > 0) {
    for (int i = 0; i < 2; i++) {
      int pid = fork();

      if (pid < 0) {
        event = 'R';
        write(fd, &event, 1);
      } else if (pid == 0) {
        stress(depth - 1, fd);
        exit(0);
      } else {
        children++;
        event = 'C';
        write(fd, &event, 1);
      }
    }
  }

  // Keep processes alive to create concurrent pressure.
  pause(30);

  for (int i = 0; i < children; i++)
    wait(&status);
}

int
main(void)
{
  int id, pid, fd[2], status;
  int failures = 0;
  int peak = 0;
  int accepted = 0;
  int rejected = 0;
  char event;
  struct container_info info;

  printf("=== T06 FORK BOMB CONTAINMENT TEST ===\n");

  id = cont_create(LIMIT);

  if (id < 0 ||
      cont_setroot(id, "/") < 0 ||
      pipe(fd) < 0) {
    printf("FAIL: setup\n");
    exit(1);
  }

  pid = fork();

  if (pid < 0)
    exit(1);

  if (pid == 0) {
    close(fd[0]);

    if (cont_enter(id) < 0)
      exit(1);

    stress(MAX_DEPTH, fd[1]);

    close(fd[1]);
    exit(0);
  }

  close(fd[1]);

  // Host independently observes the container.
  for (int i = 0; i < 15; i++) {
    if (cont_info(id, &info) < 0) {
      failures++;
      break;
    }

    if (info.proc_count > peak)
      peak = info.proc_count;

    if (info.proc_count > LIMIT)
      failures++;

    printf("FORK_SAMPLE: proc=%d limit=%d pages=%d\n",
           info.proc_count, LIMIT, info.pages_used);

    printf("HOST_PROGRESS: tick=%d\n", uptime());

    pause(5);
  }

  while (read(fd[0], &event, 1) == 1) {
    if (event == 'C')
      accepted++;
    else if (event == 'R')
      rejected++;
  }

  close(fd[0]);

  if (wait(&status) != pid || status != 0)
    failures++;

  printf("FORK_RESULT: accepted=%d rejected=%d peak=%d\n",
         accepted, rejected, peak);

  if (rejected > 0 &&
      peak > 0 &&
      peak <= LIMIT) {
    printf("PASS: bounded fork pressure contained\n");
  } else {
    printf("FAIL: containment not verified\n");
    failures++;
  }

  if (cont_info(id, &info) == 0 &&
      info.proc_count == 0 &&
      info.pages_used == 0) {
    printf("PASS: process and memory recovered\n");
  } else {
    printf("FAIL: resource recovery\n");
    failures++;
  }

  printf("=== T06 FORK BOMB TEST: %d failures ===\n",
         failures);

  exit(failures ? 1 : 0);
}
