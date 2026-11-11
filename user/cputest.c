#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEST_TICKS 200
#define WINDOW_TICKS 20
#define BATCH 1000

struct cpu_result {
  int id;
  int zero_windows;
  uint64 iterations;
  uint64 window_iterations[TEST_TICKS / WINDOW_TICKS];
};

static int target_container_id;

static int
print_cpu_info(const char *label)
{
  struct container_info info;

  if (cont_info(target_container_id, &info) < 0) {
    printf("FAIL: cont_info %s\n", label);
    return -1;
  }
  printf("cont_info %s: id=%d period=%ld quota=%ld used=%ld\n", label,
         info.id, (long)info.cpu_period_ticks, (long)info.cpu_quota_ticks,
         (long)info.cpu_used_ticks);
  return 0;
}

static void
run_load(int id, int result_fd)
{
  struct cpu_result result;
  uint64 window_iterations = 0;
  int window_index = 0;
  int start, end, next, now;

  if (id == 1 && cont_enter(target_container_id) < 0) {
    printf("cputest: cont_enter failed\n");
    exit(1);
  }

  result.id = id;
  result.zero_windows = 0;
  result.iterations = 0;
  start = uptime();
  end = start + TEST_TICKS;
  next = start + WINDOW_TICKS;
  now = start;

  while (now < end) {
    for (int i = 0; i < BATCH; i++) {
      result.iterations++;
      window_iterations++;
    }
    now = uptime();
    while (now >= next && next <= end) {
      if (id == 1) {
        if (window_iterations == 0)
          result.zero_windows++;
        if (window_index < TEST_TICKS / WINDOW_TICKS)
          result.window_iterations[window_index++] = window_iterations;
      }
      window_iterations = 0;
      next += WINDOW_TICKS;
    }
  }

  if (write(result_fd, &result, sizeof(result)) != sizeof(result))
    printf("cputest: result write failed for worker %d\n", id);
  exit(0);
}

int
main(void)
{
  int pipefd[2];
  int pid_a, pid_b;
  int monitor_start, monitor_end, last_progress, last_info;
  struct cpu_result result;
  int received = 0, bad = 0;
  uint64 a_iterations = 0, b_iterations = 0;
  int a_zero_windows = -1;

  printf("=== CPU QUOTA TEST (%d ticks, period %d, quota 5) ===\n",
         TEST_TICKS, WINDOW_TICKS);
  target_container_id = cont_create(0);
  if (target_container_id < 0 ||
      cont_set_cpu(target_container_id, 5, WINDOW_TICKS) < 0) {
    printf("FAIL: create/configure CPU quota\n");
    exit(1);
  }
  if (print_cpu_info("before workload") < 0)
    bad = 1;
  if (pipe(pipefd) < 0) {
    printf("FAIL: create result pipe\n");
    exit(1);
  }

  pid_a = fork();
  if (pid_a == 0) {
    close(pipefd[0]);
    run_load(1, pipefd[1]);
  }
  if (pid_a < 0) {
    printf("FAIL: fork quota worker\n");
    exit(1);
  }

  pid_b = fork();
  if (pid_b == 0) {
    close(pipefd[0]);
    run_load(2, pipefd[1]);
  }
  if (pid_b < 0) {
    printf("FAIL: fork host worker\n");
    exit(1);
  }

  close(pipefd[1]);
  monitor_start = uptime();
  monitor_end = monitor_start + TEST_TICKS + WINDOW_TICKS;
  last_progress = monitor_start;
  last_info = monitor_start + WINDOW_TICKS / 2;
  while (uptime() < monitor_end) {
    int now = uptime();
    if (now - last_progress >= WINDOW_TICKS) {
      printf("HOST_PROGRESS: tick %d\n", now);
      last_progress = now;
    }
    if (now >= last_info) {
      if (print_cpu_info("during workload") < 0)
        bad = 1;
      last_info += WINDOW_TICKS;
    }
    pause(1);
  }

  while (received < 2) {
    int n = read(pipefd[0], &result, sizeof(result));
    if (n != sizeof(result)) {
      printf("FAIL: read worker result (%d bytes)\n", n);
      bad = 1;
      break;
    }
    if (result.id == 1) {
      a_iterations = result.iterations;
      a_zero_windows = result.zero_windows;
      for (int i = 0; i < TEST_TICKS / WINDOW_TICKS; i++)
        printf("A window %d (%d ticks): %ld iterations\n", i + 1,
               WINDOW_TICKS, (long)result.window_iterations[i]);
    } else {
      b_iterations = result.iterations;
    }
    received++;
  }
  close(pipefd[0]);
  wait(0);
  wait(0);

  printf("RESULT: container A iterations=%ld zero_windows=%d\n",
         (long)a_iterations, a_zero_windows);
  printf("RESULT: host B iterations=%ld\n", (long)b_iterations);
  if (print_cpu_info("after workload") < 0)
    bad = 1;
  if (a_iterations == 0 || b_iterations == 0 || a_iterations >= b_iterations) {
    printf("FAIL: quota worker was not measurably limited\n");
    bad = 1;
  }
  if (a_zero_windows != 0) {
    printf("FAIL: a 20-tick quota window had no worker progress\n");
    bad = 1;
  }
  if (bad)
    printf("=== CPU QUOTA TEST FAILED ===\n");
  else
    printf("=== CPU QUOTA TEST PASS ===\n");
  exit(bad);
}
