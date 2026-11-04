#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: chroottest path\n");
    exit(1);
  }

  printf("Changing root to: %s\n", argv[1]);

  if (chroot(argv[1]) < 0) {
    printf("chroot failed\n");
    exit(1);
  }

  printf("chroot succeeded\n");

  if (chdir("/") < 0) {
    printf("chdir failed\n");
    exit(1);
  }

  printf("chdir to new root succeeded\n");

  exit(0);
}