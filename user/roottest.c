
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int failures;

void
test_result(char *name, int ok)
{
  if (ok) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s\n", name);
    failures++;
  }
}

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
      failures++;
    }
  } else {
    if (fd < 0) {
      printf("PASS: blocked %s\n", path);
    } else {
      printf("FAIL: accessible %s\n", path);
      failures++;
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
    failures++;
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
  else {
    printf("FAIL: exec inside root\n");
    failures++;
  }
}

void
test_exec_outside(void)
{
  int pid, status;
  char *argv[] = {"outside_echo", "OUTSIDE_EXEC_SHOULD_NOT_RUN", 0};

  pid = fork();

  if (pid < 0) {
    printf("FAIL: fork for outside exec\n");
    failures++;
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

void
test_exec_outside_probe(void)
{
  int pid, status;
  char *argv[] = {"outsideexec", 0};

  pid = fork();
  if (pid < 0) {
    test_result("fork for outside exec probe", 0);
    return;
  }
  if (pid == 0) {
    exec("/outsideexec", argv);
    // The fixture installs this executable outside /c1. Failure to resolve
    // it from the container root is the expected result.
    exit(0);
  }

  if (wait(&status) < 0)
    status = -1;
  test_result("exec outside root blocked", status == 0);
}

void
test_create_link_unlink(void)
{
  int fd;

  fd = open("/created.txt", O_CREATE | O_WRONLY);
  if (fd >= 0) {
    if (write(fd, "created\n", 8) == 8)
      test_result("create file inside root", 1);
    else
      test_result("create file inside root", 0);
    close(fd);
    test_result("unlink file inside root", unlink("/created.txt") == 0);
  } else {
    test_result("create file inside root", 0);
  }

  test_result("link file inside root", link("/test.txt", "/linked.txt") == 0);
  test_open("/linked.txt", 1);
  test_result("unlink linked file inside root", unlink("/linked.txt") == 0);

  // Parent traversal is clamped at the process root. The destination is
  // therefore created inside /c1, not in the host root.
  test_result("link with parent traversal stays in root",
              link("/test.txt", "/../../bounded_link.txt") == 0);
  test_open("/bounded_link.txt", 1);
  test_result("unlink bounded link", unlink("/bounded_link.txt") == 0);

  test_result("unlink outside file remains blocked",
              unlink("/../../outside.txt") < 0);
}

void
test_nameiparent_dotdot(void)
{
  test_result("mkdir final dotdot rejected", mkdir("/..") < 0);
  test_result("create directory for final dotdot test", mkdir("/a") == 0);
  test_result("unlink final dotdot rejected", unlink("a/..") < 0);
  test_result("remove test directory", unlink("/a") == 0);
}

void
test_fork_inherits_root(void)
{
  int pid, status;

  pid = fork();
  if (pid < 0) {
    test_result("fork with container root", 0);
    return;
  }
  if (pid == 0) {
    int fd = open("/test.txt", O_RDONLY);
    if (fd < 0)
      exit(1);
    close(fd);
    fd = open("/outside.txt", O_RDONLY);
    if (fd >= 0) {
      close(fd);
      exit(2);
    }
    exit(0);
  }

  if (wait(&status) < 0)
    status = -1;
  test_result("forked child retains root boundary", status == 0);
}

int
main(int argc, char *argv[])
{
  int fd, container_id, root_ready = 0;
  char data[32];
  int n;

  printf("=== ROOT DIRECTORY TEST ===\n");

  // Exercise an already-open descriptor across chroot and prepare the
  // container root while this process still uses the host root.
  fd = open("/outside.txt", O_RDONLY);
  test_result("open outside file before chroot", fd >= 0);

  container_id = cont_create(8);
  if (container_id >= 0) {
    root_ready = cont_setroot(container_id, "/c1") == 0;
    test_result("set container root", root_ready);
    test_result("container root can only be set once",
                cont_setroot(container_id, "/") < 0);
  } else {
    test_result("create root test container", 0);
  }

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

  if (fd >= 0) {
    n = read(fd, data, sizeof(data) - 1);
    if (n >= 0)
      data[n] = 0;
    test_result("pre-opened fd remains usable after chroot",
                n == 7 && strcmp(data, "secret\n") == 0);
    close(fd);
  }

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

  test_create_link_unlink();
  test_nameiparent_dotdot();
  test_fork_inherits_root();

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
  test_exec_outside_probe();

  // Enter only after all host-only setup and per-process root checks.
  if (root_ready && cont_enter(container_id) == 0) {
    test_open("/test.txt", 1);
    test_open("/outside.txt", 0);
    test_result("cont_enter applies container root", 1);
  } else {
    test_result("cont_enter applies container root", 0);
  }

  printf("=== ROOT TEST FINISHED: %d failures ===\n", failures);

  exit(failures == 0 ? 0 : 1);
}
