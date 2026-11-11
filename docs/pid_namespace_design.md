# SCRUM-90 - PID Namespace Survey and Design

## 1. Objective

Design a PID namespace mechanism for xv6-riscv.

Goal:
- Each container sees its own PID namespace.
- The first process in each container sees PID 1.
- Preserve global PID internally in the kernel.
- Prevent processes from controlling processes in other containers.

Status: Design only. Not implemented.

## 2. Current architecture

The project already supports:
- Container process membership.
- Process quota.
- Container-level filesystem root.
- CPU quota prototype.
- Memory quota groundwork.

The kernel currently uses a global PID for process management.

PID namespace has not yet been implemented.

## 3. Proposed PID mapping

Maintain two identifiers:

- Global PID: unique throughout the kernel.
- Virtual PID: unique within one container.

Example:

Container A:
Global PID 14 -> Virtual PID 1
Global PID 16 -> Virtual PID 2

Container B:
Global PID 15 -> Virtual PID 1
Global PID 17 -> Virtual PID 2

These PID numbers are illustrative only.

## 4. Proposed data structures

Add to struct proc:

    int virtual_pid;

Add to struct container:

    int next_virtual_pid;

The global PID remains stored in p->pid.

The mapping must be protected against concurrent forks.

## 5. PID allocation

When a process becomes the first member of a container:
- Assign virtual PID 1.
- Record the container namespace.

When fork creates a child:
- Child inherits the container.
- Allocate a new virtual PID.
- Preserve the global PID.

Do not reuse virtual PID values while another live process
in the same container still owns them.

## 6. System call impact

### getpid

Return virtual PID to processes inside a container.
Keep global PID internally for kernel operations.

### kill

Translate virtual PID to global process identity.
Only allow targeting processes in the same namespace,
unless an explicit host policy permits otherwise.

### wait / kwait

Preserve real parent-child relationships.
Return the child's PID in the caller's namespace.
Avoid confusing virtual PID with global PID.

### reparent

Define namespace-init behavior when virtual PID 1 exits.
Prevent orphaned processes from escaping namespace rules.

### procdump

Specify whether host debugging output uses global PID
or container-local PID.

## 7. Locking and lifecycle

Review:
- PID allocation under concurrent fork.
- Interaction with p->lock and wait_lock.
- PID allocation/reuse after process exit.
- Cleanup when processes become ZOMBIE.
- Container lifecycle and namespace-init exit.
- Changing container membership with cont_enter.

A design decision is required for processes that already
exist before entering a container.

## 8. T03 test plan

Test objective:
Two independent containers should each see PID 1.

Suggested test:
1. Create container A.
2. Start its first process.
3. Check getpid() returns 1.
4. Create container B.
5. Start its first process.
6. Check getpid() returns 1.
7. Verify global PIDs remain unique.
8. Verify kill cannot access the other container.
9. Verify fork/wait still works correctly.

Expected result:
Both containers observe PID 1 locally without sharing
the same global process identity.

Current status: NOT IMPLEMENTED / NOT RUN.

## 9. Implementation estimate

Major work packages:
- Survey all PID references.
- Define namespace creation and membership rules.
- Implement virtual PID allocation.
- Update getpid, kill, wait and process lookup.
- Handle reparent and PID 1 lifecycle.
- Add concurrency and isolation tests.

The team must estimate effort after reviewing the
full PID-related call paths.

## 10. Risks and future work

- Global and virtual PID confusion.
- PID reuse races.
- Incorrect parent-child handling.
- Cross-container kill.
- Namespace-init process exit.
- Scheduler and process quota regression.

Conclusion:
PID namespace is a future implementation task.
This document records the proposed design only.
