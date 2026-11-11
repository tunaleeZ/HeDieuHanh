#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
write_file(char *path, char *data)
{
  int fd, len, off, n;

  fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0)
    return -1;

  len = strlen(data);
  off = 0;
  while (off < len) {
    n = write(fd, data + off, len - off);
    if (n <= 0) {
      close(fd);
      return -1;
    }
    off += n;
  }

  close(fd);
  return 0;
}

static int
copy_file(char *srcpath, char *dstpath)
{
  char buf[512];
  int src, dst, n, off, written;

  src = open(srcpath, O_RDONLY);
  if (src < 0)
    return -1;
  dst = open(dstpath, O_CREATE | O_WRONLY | O_TRUNC);
  if (dst < 0) {
    close(src);
    return -1;
  }

  while ((n = read(src, buf, sizeof(buf))) > 0) {
    off = 0;
    while (off < n) {
      written = write(dst, buf + off, n - off);
      if (written <= 0) {
        close(src);
        close(dst);
        return -1;
      }
      off += written;
    }
  }

  close(src);
  close(dst);
  return n < 0 ? -1 : 0;
}

int
main(void)
{
  if (mkdir("/c1") < 0) {
    printf("FAIL: mkdir /c1\n");
    exit(1);
  }
  if (write_file("/c1/test.txt", "hello\n") < 0) {
    printf("FAIL: create /c1/test.txt\n");
    exit(1);
  }
  if (write_file("/outside.txt", "secret\n") < 0) {
    printf("FAIL: create /outside.txt\n");
    exit(1);
  }
  if (copy_file("/echo", "/c1/echo") < 0) {
    printf("FAIL: copy /echo to /c1/echo\n");
    exit(1);
  }

  printf("PASS: /c1 root fixture ready\n");
  exit(0);
}
