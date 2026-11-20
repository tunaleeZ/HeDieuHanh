#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

static void
usage(void)
{
  fprintf(2, "Usage: container-run [-p max_proc] [-c quota:period] [-m max_pages] [-v] <root> <cmd> [args...]\n");
  fprintf(2, "       container-run -i <id> [-v] <cmd> [args...]\n");
  fprintf(2, "       container-run -l\n");
}

// Parse non-negative integer. Returns -1 on invalid character or overflow.
static int
parse_nonneg_int(const char *s)
{
  if (!s || *s == '\0')
    return -1;
  int val = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] < '0' || s[i] > '9')
      return -1;
    val = val * 10 + (s[i] - '0');
    if (val < 0)
      return -1;
  }
  return val;
}

// Parse -c <quota>:<period>. Returns 0 on success, -1 on error.
// Quota 0 is valid and means unlimited CPU.
static int
parse_cpu_arg(const char *arg, int *quota, int *period)
{
  char buf[32];
  int len = strlen(arg);
  if (len >= sizeof(buf) || len == 0)
    return -1;

  char *colon = strchr(arg, ':');
  if (!colon)
    return -1;

  int qlen = colon - arg;
  if (qlen == 0 || qlen >= sizeof(buf))
    return -1;

  memmove(buf, arg, qlen);
  buf[qlen] = '\0';

  const char *pstr = colon + 1;
  if (*pstr == '\0')
    return -1;

  int q = parse_nonneg_int(buf);
  int p = parse_nonneg_int(pstr);

  if (q < 0 || p <= 0 || q > p)
    return -1;

  *quota = q;
  *period = p;
  return 0;
}

// List all 8 container slots.
static void
list_containers(void)
{
  printf("ID  ACTIVE  PROCS/MAX  CPU_QUOTA/PERIOD  MEM_PAGES/MAX\n");
  for (int i = 0; i < 8; i++) {
    struct container_info info;
    if (cont_info(i, &info) == 0) {
      if (info.active) {
        printf("%d   YES     %d/%d        %d/%d             %d/%d\n",
               info.id,
               info.proc_count, info.max_proc,
               (int)info.cpu_quota_ticks, (int)info.cpu_period_ticks,
               info.pages_used, info.max_pages);
      } else {
        printf("%d   NO      -          -                 -\n", info.id);
      }
    }
  }
}

// Construct host path: <root> + "/" + <cmd> with slash normalization.
static void
build_host_path(char *dst, int maxlen, const char *root, const char *cmd)
{
  int rlen = strlen(root);
  int clen = strlen(cmd);
  int pos = 0;

  for (int i = 0; i < rlen && pos < maxlen - 1; i++) {
    dst[pos++] = root[i];
  }

  // Remove trailing slashes if pos > 1
  while (pos > 1 && dst[pos - 1] == '/') {
    pos--;
  }

  int cstart = 0;
  if (pos == 1 && dst[0] == '/') {
    // root is "/", skip leading slash of cmd
    while (cmd[cstart] == '/')
      cstart++;
  } else {
    // root is not "/", ensure a single separating slash
    if (cmd[0] != '/') {
      if (pos < maxlen - 1)
        dst[pos++] = '/';
    }
  }

  for (int i = cstart; i < clen && pos < maxlen - 1; i++) {
    dst[pos++] = cmd[i];
  }
  dst[pos] = '\0';
}

int
main(int argc, char *argv[])
{
  int max_proc = 0;
  int cpu_quota = 0;
  int cpu_period = 20;
  int has_cpu = 0;
  int max_pages = 0;
  int has_mem = 0;
  int verbose = 0;
  int reuse_id = 0;
  int do_list = 0;

  // Step 0: Parse arguments
  int i = 1;
  while (i < argc && argv[i][0] == '-') {
    if (strcmp(argv[i], "-l") == 0) {
      do_list = 1;
      i++;
      break;
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = 1;
      i++;
    } else if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 >= argc) {
        usage();
        exit(1);
      }
      max_proc = parse_nonneg_int(argv[i + 1]);
      if (max_proc < 0) {
        fprintf(2, "container-run: invalid max_proc '%s'\n", argv[i + 1]);
        exit(1);
      }
      i += 2;
    } else if (strcmp(argv[i], "-c") == 0) {
      if (i + 1 >= argc) {
        usage();
        exit(1);
      }
      if (parse_cpu_arg(argv[i + 1], &cpu_quota, &cpu_period) < 0) {
        fprintf(2, "container-run: invalid cpu quota:period '%s'\n", argv[i + 1]);
        exit(1);
      }
      has_cpu = 1;
      i += 2;
    } else if (strcmp(argv[i], "-m") == 0) {
      if (i + 1 >= argc) {
        usage();
        exit(1);
      }
      max_pages = parse_nonneg_int(argv[i + 1]);
      if (max_pages < 0) {
        fprintf(2, "container-run: invalid max_pages '%s'\n", argv[i + 1]);
        exit(1);
      }
      has_mem = 1;
      i += 2;
    } else if (strcmp(argv[i], "-i") == 0) {
      if (i + 1 >= argc) {
        usage();
        exit(1);
      }
      reuse_id = parse_nonneg_int(argv[i + 1]);
      if (reuse_id <= 0 || reuse_id >= 8) {
        fprintf(2, "container-run: invalid container id '%s' (must be 1..7)\n", argv[i + 1]);
        exit(1);
      }
      i += 2;
    } else {
      fprintf(2, "container-run: unknown option '%s'\n", argv[i]);
      usage();
      exit(1);
    }
  }

  // Handle -l
  if (do_list) {
    list_containers();
    exit(0);
  }

  // Check flag conflicts for -i
  if (reuse_id > 0) {
    if (max_proc > 0 || has_cpu || has_mem) {
      fprintf(2, "container-run: -i cannot be combined with resource options (-p, -c, -m)\n");
      exit(1);
    }
  }

  // Positional arguments
  const char *root = 0;
  const char *cmd = 0;
  int cmd_idx = 0;

  if (reuse_id > 0) {
    if (i >= argc) {
      usage();
      exit(1);
    }
    cmd = argv[i];
    cmd_idx = i;
  } else {
    if (i + 1 >= argc) {
      usage();
      exit(1);
    }
    root = argv[i];
    cmd = argv[i + 1];
    cmd_idx = i + 1;
  }

  // Prepare child arguments
  char *child_argv[MAXARG];
  int child_argc = 0;
  child_argv[child_argc++] = (char *)cmd;
  for (int j = cmd_idx + 1; j < argc; j++) {
    if (child_argc >= MAXARG - 1) {
      fprintf(2, "container-run: too many arguments (max %d)\n", MAXARG - 1);
      exit(1);
    }
    child_argv[child_argc++] = argv[j];
  }
  child_argv[child_argc] = 0;

  int cid = reuse_id;

  if (reuse_id > 0) {
    // Validate that the existing container is active
    struct container_info info;
    if (cont_info(reuse_id, &info) < 0 || !info.active) {
      fprintf(2, "container-run: container %d is not active\n", reuse_id);
      exit(1);
    }
  } else {
    // Step 1: Pre-flight check on host
    struct stat st;
    int fd = open(root, O_RDONLY);
    if (fd < 0) {
      fprintf(2, "container-run: root '%s' does not exist\n", root);
      exit(1);
    }
    if (fstat(fd, &st) < 0 || st.type != T_DIR) {
      close(fd);
      fprintf(2, "container-run: root '%s' is not a directory\n", root);
      exit(1);
    }
    close(fd);

    // Verify binary exists in root from host
    char host_path[MAXPATH];
    build_host_path(host_path, sizeof(host_path), root, cmd);
    fd = open(host_path, O_RDONLY);
    if (fd < 0) {
      fprintf(2, "container-run: command '%s' not found in root '%s'\n", cmd, root);
      exit(1);
    }
    if (fstat(fd, &st) < 0 || st.type != T_FILE) {
      close(fd);
      fprintf(2, "container-run: command '%s' in root '%s' is not a file\n", cmd, root);
      exit(1);
    }
    close(fd);

    // Step 2: Create container
    cid = cont_create(max_proc);
    if (cid < 0) {
      fprintf(2, "container-run: cannot create container: no free slots available or not host\n");
      exit(1);
    }

    if (verbose) {
      printf("container-run: container %d created\n", cid);
    }

    // Step 3: Configure resources
    if (has_mem) {
      if (cont_set_mem(cid, max_pages) < 0) {
        fprintf(2, "container-run: cont_set_mem failed for container %d\n", cid);
        exit(1);
      }
    }

    if (has_cpu) {
      if (cont_set_cpu(cid, cpu_quota, cpu_period) < 0) {
        fprintf(2, "container-run: cont_set_cpu failed for container %d\n", cid);
        exit(1);
      }
    }

    // Step 4: Set root
    if (cont_setroot(cid, root) < 0) {
      fprintf(2, "container-run: cont_setroot failed for container %d with root '%s'\n", cid, root);
      exit(1);
    }
  }

  // Step 5: Fork child process
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "container-run: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process: enter container
    if (cont_enter(cid) < 0) {
      fprintf(2, "container-run: cont_enter failed for container %d\n", cid);
      exit(1);
    }
    // Execute command within container root
    exec(cmd, child_argv);
    fprintf(2, "container-run: exec '%s' failed in container %d\n", cmd, cid);
    exit(127);
  }

  // Step 6: Parent process (Host) - Never calls cont_enter
  int status = 0;
  int wpid = wait(&status);
  if (wpid < 0) {
    fprintf(2, "container-run: wait failed\n");
    exit(1);
  }

  if (verbose) {
    struct container_info info;
    if (cont_info(cid, &info) == 0) {
      printf("container-run: container %d finished (proc_count=%d, cpu_used=%d, pages_used=%d)\n",
             cid, info.proc_count, (int)info.cpu_used_ticks, info.pages_used);
    }
  }

  if (status == -1) {
    fprintf(2, "container-run: process in container %d terminated abnormally or killed\n", cid);
    exit(1);
  }

  exit(status);
}
