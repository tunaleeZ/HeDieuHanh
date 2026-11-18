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

## 11. SCRUM96 implementation decisions

Các quyết định dưới đây được duyệt trước khi triển khai SCRUM96:

- `proc.pid` tiếp tục là global PID duy nhất, dùng cho quản lý nội bộ và debug kernel.
- Host namespace (container 0) giữ PID nhìn thấy bằng global PID để tương thích hành vi hiện tại.
- Process host đầu tiên gọi `cont_enter()` vào một container root đã cấu hình trở thành namespace-init với virtual PID 1. `cont_enter()` bị từ chối nếu process còn child, tránh quan hệ parent-child bị chia giữa hai namespace.
- `fork()` trong container cấp virtual PID mới; `fork()` ở host tiếp tục trả global PID. `wait()` trong cùng namespace trả virtual PID. Host parent đang chờ process đã chuyển namespace nhận global PID của process đó.
- `kill()` trong container chỉ tra process thuộc cùng container bằng virtual PID. Host giữ quyền quản lý hiện tại bằng global PID.
- Virtual PID tăng đơn điệu, không tái sử dụng trong vòng đời container. `next_virtual_pid == 0` biểu thị bộ cấp phát đã đóng hoặc cạn; PID dương lớn nhất được phép là `0x7fffffff`.
- Khi namespace-init thoát, namespace bị đóng, các thành viên còn lại bị đánh dấu killed, và orphan được chuyển cho global init để reap. Không cho fork/entry mới và không tái sử dụng container; repo hiện chưa có `cont_destroy()`.
- Orphan trong namespace còn PID 1 được chuyển cho namespace-init. Process cha ở namespace khác chỉ có thể là host parent của process đã `cont_enter()`; host `wait()` nhận global PID.
- Cấp phát virtual PID được tuần tự hóa bằng `container.lock`. Lookup process giữ `p->lock` khi so sánh PID/container và thay đổi trạng thái. Khi cần nhiều lock, giữ thứ tự `wait_lock -> p->lock -> container->lock`; không giữ container lock qua thao tác có thể sleep.

SCRUM96 gộp phần cấp phát entry/fork tối thiểu vào sub-task 2.1 để kiểm tra được tính duy nhất virtual PID trước khi commit. Sub-task 2.2 tập trung vào lookup và dịch PID; sub-task 2.3 tích hợp các syscall.

## 12. SCRUM96 PID translation helpers

`process_visible_pid(viewer, target)` assumes the caller holds `target->lock`. It returns the target virtual PID to a same-container viewer, the target global PID to a host viewer, or -1 when a non-host viewer cannot observe the target. `global_pid_from_visible(viewer, pid)` passes a positive host PID through unchanged; for a non-host viewer it scans live processes while holding one process lock at a time and returns the matching global PID only. It never returns or retains a process pointer, so callers must perform a fresh locked lookup before changing process state. Syscall integration is part of SCRUM96 sub-task 2.3.

## 13. SCRUM96 syscall integration and namespace-init exit

`getpid()` returns `proc.virtual_pid`. `wait()` preserves parent-child scanning under `wait_lock`, and translates the zombie child's PID while holding the child lock. This also means a host parent that created a process before it entered a container continues to reap it using its global PID. `kill()` translates the caller-visible PID to a global PID, then locks the matched process before changing its state. Non-host lookup is restricted to the caller's container; host PID behavior remains global.

When a non-host namespace-init exits, it sets `next_virtual_pid` to zero under the container lock, then marks remaining live members killed and wakes sleepers. Its orphans are reassigned to global init. When another member exits while a live PID 1 exists, its orphans are reassigned to that namespace-init; otherwise they go to global init. No container lock is held while scanning process locks. `cont_enter()` acquires `p->lock` before `container->lock`, and publishes `p->container`/`p->virtual_pid` while both locks are held. Thus entry is visible to the namespace-init exit scan, or loses the race to the closed allocator; it cannot enter after that scan unnoticed. Runtime regression and namespace isolation evidence is recorded in SCRUM96 sub-task 2.4.
