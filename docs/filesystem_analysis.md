# Filesystem Analysis in xv6-riscv

## 1. Objective

This document investigates xv6-riscv filesystem path resolution, file-related system calls, and executable loading. The findings provide a foundation for designing filesystem isolation for lightweight containers.

## 2. Relevant Source Files

| File | Responsibility |
|---|---|
| `kernel/fs.c` | Inode operations, directory lookup, and path resolution |
| `kernel/sysfile.c` | File-related system calls, including `open()` and `chdir()` |
| `kernel/exec.c` | Loads executable ELF files into a process |
| `kernel/proc.c` | Stores and manages the process working directory |

## 3. Path Resolution

### 3.1. `skipelem()`

The `skipelem()` function in `kernel/fs.c` skips leading slashes, extracts the next path component, and returns the remaining path. It parses the path string but does not itself verify whether the corresponding filesystem entry exists.

### 3.2. `namex()`

The `namex()` function resolves path components by traversing directory inodes.

The inspected implementation selects its initial inode as follows:

- Absolute paths beginning with `/` start at `iget(ROOTDEV, ROOTINO)`.
- Relative paths start at a reference to the current process's working directory using `idup(myproc()->cwd)`.

The function processes path components, checks that intermediate inodes are directories with valid links, and uses `dirlookup()` to locate the next component.

This behavior identifies `namex()` as a central location to investigate when introducing a container-specific filesystem root.

### 3.3. `namei()` and `nameiparent()`

`namei()` calls `namex()` to resolve a complete path to an inode.

`nameiparent()` calls `namex()` in parent-lookup mode. It returns the parent directory inode and places the final path component in the supplied name buffer.

These functions are relevant to multiple filesystem operations, not only opening existing files.

## 4. File-Related System Calls

### 4.1. `sys_open()`

The inspected `sys_open()` implementation obtains a path from user space. With `O_CREATE`, it calls `create()`. Otherwise, it calls `namei()` to locate the existing inode.

It then allocates a file object and file descriptor, sets the relevant read/write properties, and returns the descriptor.

### 4.2. `sys_chdir()`

`sys_chdir()` resolves the requested path using `namei()`, verifies that the inode is a directory, and replaces the process's current working directory (`cwd`).

Changing `cwd` does not, by itself, change the filesystem root used for absolute paths in the inspected `namex()` implementation.

### 4.3. `kexec()`

The `kexec()` function in `kernel/exec.c` resolves the executable path using `namei()`. It reads and validates the ELF header, loads program segments, prepares the user stack, and replaces the process's user address space.

Filesystem isolation must account for executable loading as well as ordinary file operations.

## 5. Initial Container Filesystem Design

The inspected implementation uses a global filesystem root for absolute paths and the process's `cwd` for relative paths. The inspected `struct proc` contains `cwd` but does not contain an explicit container-specific root field.

A preliminary design should investigate:

1. How to represent and initialize a root directory for each container.
2. How to make absolute paths start at the container root.
3. How to prevent traversal above the container root through `..`.
4. How to preserve the appropriate root and working directory when a process forks.
5. How to apply path restrictions consistently across file creation, deletion, directory changes, and executable loading.

These are design considerations; container filesystem isolation has not yet been implemented.

## 6. Findings and Limitations

The inspected source confirms that `namex()` is a shared path-resolution mechanism used through `namei()` and `nameiparent()`. The inspected `sys_open()`, `sys_chdir()`, and `kexec()` functions rely on this path-resolution mechanism directly or indirectly.

Further inspection is needed to enumerate every path-based filesystem operation and verify all relevant parent-directory lookup paths before finalizing the implementation plan.

## 7. Evidence

Source locations inspected:

- `kernel/fs.c`: `skipelem()`, `namex()`, `namei()`, and `nameiparent()`.
- `kernel/sysfile.c`: `sys_open()` and `sys_chdir()`.
- `kernel/exec.c`: `kexec()`.
- `kernel/proc.c`: process working-directory initialization and inheritance.

## 8. Next Steps

- Inspect `create()` and the remaining path-based filesystem system calls.
- Confirm how root and working-directory references should be managed.
- Finalize the proposed filesystem isolation design before modifying kernel code.

## 9. Path-Based System Calls

The source-code inspection identified the following filesystem operations:

- `sys_link()` resolves the source path with `namei()` and the destination parent with `nameiparent()`.
- `sys_unlink()` resolves the parent directory with `nameiparent()` before removing a directory entry.
- `create()` resolves the parent directory with `nameiparent()` and creates an inode for the requested file type.
- `sys_open()` uses `create()` for `O_CREATE` and `namei()` for existing paths.
- `sys_mkdir()` and `sys_mknod()` use `create()` to create directories and device nodes.
- `sys_chdir()` uses `namei()` to resolve the target directory.
- `kexec()` uses `namei()` to resolve the executable path.

### Design Implications

The existing path-resolution mechanism uses the global filesystem root for absolute paths and the process working directory for relative paths. The inspected `struct proc` contains `cwd` but no separate container root.

A container filesystem design must therefore consider:

1. Maintaining a root inode for each container, or for each process associated with a container.
2. Starting absolute-path resolution from the container root.
3. Preventing traversal above the container root through `..`.
4. Preserving and correctly managing root and working-directory references across `fork()` and process termination.
5. Auditing all path-based system calls, including file creation, deletion, linking, directory changes and executable loading.

These points are design proposals only. No filesystem isolation mechanism has been implemented yet.