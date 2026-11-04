
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void
test_open(char *path, int expected)
{
  int fd;

  fd = open(path, O_RDONLY);

  if (expected) {
    if (fd >= 0) {
      printf("PASS: open %s\n", path);
      close(fd);
    } else {
      printf("FAIL: open %s\n", path);
    }
  } else {
    if (fd < 0) {
      printf("PASS: blocked %s\n", path);
    } else {
      printf("FAIL: accessible %s\n", path);
      close(fd);
    }
  }
}

void
test_exec_inside(void)
{
  int pid, status;
  char *argv[] = {"echo", "INSIDE_EXEC_OK", 0};

  pid = fork();

  if (pid < 0) {
    printf("FAIL: fork for inside exec\n");
    return;
  }

  if (pid == 0) {
    exec("/echo", argv);
    printf("FAIL: exec /echo\n");
    exit(1);
  }

  wait(&status);

  if (status == 0)
    printf("PASS: exec inside root\n");
  else
    printf("FAIL: exec inside root\n");
}

void
test_exec_outside(void)
{
  int pid, status;
  char *argv[] = {"outside_echo", "OUTSIDE_EXEC_SHOULD_NOT_RUN", 0};

  pid = fork();

  if (pid < 0) {
    printf("FAIL: fork for outside exec\n");
    return;
  }

  if (pid == 0) {
    exec("/outside_echo", argv);

    // exec returns only if it fails.
    printf("PASS: blocked exec outside root\n");
    exit(0);
  }

  wait(&status);
}

int
main(int argc, char *argv[])
{
  printf("=== ROOT DIRECTORY TEST ===\n");

  // Change process root to /c1.
  if (chroot("/c1") < 0) {
    printf("FAIL: chroot /c1\n");
    exit(1);
  }

  printf("PASS: chroot /c1\n");

  if (chdir("/") < 0) {
    printf("FAIL: chdir /\n");
    exit(1);
  }

  printf("PASS: chdir /\n");

  // Test absolute and relative paths.
  test_open("/test.txt", 1);
  test_open("test.txt", 1);

  // Test parent traversal.
  test_open("/../test.txt", 1);
  test_open("/../../test.txt", 1);

  // Outside marker must not be accessible.
  test_open("/outside.txt", 0);
  test_open("/../outside.txt", 0);
  test_open("/../../outside.txt", 0);

  // chdir("..") must remain at process root.
  if (chdir("..") == 0) {
    printf("PASS: chdir .. at root\n");
    test_open("test.txt", 1);
  } else {
    printf("FAIL: chdir .. at root\n");
  }

  // Test exec.
  test_exec_inside();
  test_exec_outside();

  printf("=== ROOT TEST FINISHED ===\n");

  exit(0);
}
