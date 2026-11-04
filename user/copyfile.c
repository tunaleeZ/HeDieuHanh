#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int src, dst, n, written;
  char buf[512];

  if (argc != 3) {
    fprintf(2, "Usage: copyfile source destination\n");
    exit(1);
  }

  src = open(argv[1], O_RDONLY);
  if (src < 0) {
    fprintf(2, "Cannot open source\n");
    exit(1);
  }

  dst = open(argv[2], O_CREATE | O_WRONLY | O_TRUNC);
  if (dst < 0) {
    fprintf(2, "Cannot create destination\n");
    close(src);
    exit(1);
  }

  while ((n = read(src, buf, sizeof(buf))) > 0) {
    written = 0;

    while (written < n) {
      int m = write(dst, buf + written, n - written);

      if (m <= 0) {
        fprintf(2, "Write failed\n");
        close(src);
        close(dst);
        exit(1);
      }

      written += m;
    }
  }

  if (n < 0) {
    fprintf(2, "Read failed\n");
    close(src);
    close(dst);
    exit(1);
  }

  close(src);
  close(dst);

  printf("Copy completed\n");
  exit(0);
}

