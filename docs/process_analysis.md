# Process Management Analysis in xv6-riscv

## 1. Objective

This document investigates the process management mechanisms in xv6-riscv as the foundation for implementing lightweight containers. The analysis focuses on process structures, PID allocation, process creation and termination, and CPU scheduling.

## 2. Relevant Source Files

| File | Purpose |
|---|---|
| `kernel/proc.h` | Defines `struct proc` and process states |
| `kernel/proc.c` | Implements process allocation, creation, termination, waiting, and scheduling |

## 3. Process Structure

The `struct proc` structure in `kernel/proc.h` stores the state and resources associated with each process.

Important fields include:

- `pid`: Process identifier.
- `state`: Current process state.
- `parent`: Pointer to the parent process.
- `killed`: Indicates whether the process has been marked for termination.
- `xstate`: Exit status returned to the parent.
- `sz`: Size of the process memory.
- `pagetable`: User page table.
- `trapframe`: Stores user execution state during transitions between user space and the kernel.
- `context`: Stores context required for switching execution.
- `ofile`: Array of open file references.
- `cwd`: Current working directory.
- `name`: Process name used for debugging.

The process states are `UNUSED`, `USED`, `SLEEPING`, `RUNNABLE`, `RUNNING`, and `ZOMBIE`.

The current structure does not contain an explicit field identifying the container to which a process belongs.

## 4. PID Allocation

The `allocpid()` function in `kernel/proc.c` obtains the current value of `nextpid`, increments the counter, and protects the operation using `pid_lock`.

This implementation provides a system-wide PID allocation mechanism. A separate PID view for each container is not implemented in the inspected code.

## 5. Process Lifecycle

### 5.1 Process Allocation

`allocproc()` scans the process table for an entry in the `UNUSED` state. It assigns a PID, changes the state to `USED`, allocates a trapframe, creates a page table, and initializes the process context.

If resource allocation fails, the function calls `freeproc()` to clean up.

### 5.2 Initial User Process

`userinit()` calls `allocproc()`, assigns the resulting process to `initproc`, sets its current working directory, and changes its state to `RUNNABLE`.

### 5.3 Process Creation

`kfork()` creates a child process using `allocproc()`. It copies the parent's user memory and saved register state, duplicates references to open files, copies the working directory reference, assigns the parent pointer, and marks the child as `RUNNABLE`.

This function is a potential integration point for enforcing a per-container process limit and inheriting container membership.

### 5.4 Process Termination and Waiting

`kexit()` closes open files, releases the working directory reference, reparents abandoned children to `initproc`, and changes the current process state to `ZOMBIE`.

`kwait()` scans the process table for exited child processes. When it finds a child in the `ZOMBIE` state, it retrieves the child's PID and exit status, calls `freeproc()`, and returns the PID to the parent.

### 5.5 Resource Cleanup

`freeproc()` releases the process's trapframe and user page table, resets relevant fields, and changes the process state to `UNUSED`.

Any additional container-related resources or counters must be handled consistently during process cleanup.

### 5.6 CPU Scheduling

`scheduler()` scans the global process table for processes in the `RUNNABLE` state. It changes the selected process to `RUNNING` and switches execution using `swtch()`.

The inspected implementation does not show a container-specific CPU scheduling or CPU quota mechanism.

## 6. Initial Container Design Considerations

1. **Process membership:** Introduce a mechanism for identifying which container owns each process.
2. **Process limit:** Check the container's process count before creating a child process.
3. **PID isolation:** Distinguish internal system PIDs from the PID values visible within a container.
4. **Cleanup:** Keep process membership and container counters consistent during termination and resource reclamation.
5. **CPU limit:** Investigate how CPU usage can be measured and limited for each container.

These points are preliminary design considerations, not completed implementations.

## 7. Findings and Limitations

The inspected source code provides system-wide process management through a global process table, a shared PID allocation counter, parent-child relationships, and a scheduler.

The inspected `struct proc` does not include explicit container membership. The inspected `allocpid()` and `scheduler()` implementations also do not show per-container PID allocation or CPU quotas.

Filesystem isolation and memory-limit enforcement require separate investigations and should not be considered implemented based on this analysis alone.

## 8. Evidence and Further Work

Evidence was collected from:

- `kernel/proc.h`: `enum procstate` and `struct proc`.
- `kernel/proc.c`: `allocpid()`, `allocproc()`, `freeproc()`, `userinit()`, `kfork()`, `kexit()`, `kwait()`, and `scheduler()`.

Further work should investigate the syscall dispatch path and memory allocation mechanisms before finalizing the container architecture.

## 9. System Call Path for Process Creation

The `fork()` system call connects the user-space interface to the kernel process management implementation.

The inspected source files show the following path:

1. `user/usys.pl` contains `entry("fork")`, which generates the user-space system call wrapper.
2. `kernel/syscall.h` defines `SYS_fork` as system call number 1.
3. `kernel/syscall.c` declares `sys_fork()` and registers it in the system call dispatch table using `SYS_fork`.
4. `kernel/sysproc.c` implements `sys_fork()`, which returns the result of `kfork()`.
5. `kernel/proc.c` implements `kfork()`, which allocates a child process, copies the required process state, establishes the parent-child relationship, and marks the child as runnable.

This call path identifies `kfork()` as a potential integration point for per-container process-limit checks. Container membership must also be propagated to child processes, and any process counter must be updated consistently.

The inspected code confirms the system call registration and forwarding path. It does not establish that container-specific process limits or PID namespaces already exist.
