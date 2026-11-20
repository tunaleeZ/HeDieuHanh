# SCRUM-94 - Memory Quota Enforcement

## 1. Objective

Implement physical user-memory quota enforcement for
lightweight containers on xv6-riscv.

The container memory limit is configured through
cont_set_mem() and inspected through cont_info().

## 2. Previous Limitations

At the end of Week 3, memory quota configuration and
reservation helpers were available.

However, VM allocation and deallocation were not
connected to container memory accounting.

As a result, pages_used remained zero even when
container processes allocated memory or forked.

The reservation helper also contained a potential
signed integer overflow.

## 3. Implementation

### 3.1. Safe quota reservation

Updated container_reserve_pages() to prevent signed
integer overflow and reject requests above the
configured memory limit.

### 3.2. Physical memory accounting

Added the following helpers:

- uvm_count_pages()
- uvmalloc_accounted()
- uvmdealloc_accounted()
- uvmcopy_accounted()
- proc_freepagetable_accounted()

Accounting includes physically allocated process-image
pages, including stack guard pages.

Kernel stacks, page tables, trapframes and trampoline
mappings are excluded.

### 3.3. Process lifecycle integration

Memory accounting is integrated into:

- growproc()
- kfork()
- freeproc()
- kcontainer_enter()
- kexec()
- vmfault()

Container entry accounts for existing physical pages
before publishing the new container membership.

Fork reserves quota for copied physical pages and
releases reservations when allocation fails.

Process reaping releases physical-page accounting
before clearing container membership.

### 3.4. Lazy allocation

Lazy virtual memory reservations do not immediately
consume physical-page quota.

Physical pages are charged when vmfault() allocates
and maps them.

Access above the configured limit is rejected.

### 3.5. Exec policy

The old and new process images are accounted for
during exec().

If insufficient quota is available, exec() fails
without replacing the original process image.

## 4. Modified Files

- kernel/proc.c
- kernel/vm.c
- kernel/exec.c
- kernel/defs.h
- user/memquotatest.c
- Makefile

## 5. Test Environment

- xv6-riscv
- QEMU RISC-V
- RAM: 128 MiB
- CPU: 3 harts
- Physical page size: 4096 bytes
- Test memory quota: 32 pages (128 KiB)
- Test process quota: 8 processes

## 6. Test Results

### T07 - Allocation below quota

PASS:
- Initial physical memory: 5 pages
- Eager allocation: 5 to 9 pages
- Lazy reservation does not allocate physical pages
- Touching two lazy pages increases usage to 7 pages

### T08 - Allocation above quota

PASS:
- Eager allocation above quota rejected
- Fork rejected when memory quota is full
- Exec rejected when no free quota remains
- Container entry rejected when quota is insufficient
- Lazy quota violation terminates the offending child
- Host shell remains responsive

### T09 - Memory recovery

PASS within tested scenarios:
- Eager heap shrinking releases pages
- Lazy heap shrinking releases mapped pages
- Rejected fork does not leak quota
- Rejected exec preserves the original image
- Child memory is reclaimed after wait()
- Container pages_used returns to zero
- Container proc_count returns to zero

Final result:

MEMORY QUOTA TEST: 0 failures

Host responsiveness:

HOST_OK

## 7. Regression Testing

- forktest: PASS
- ctest: 0 failures
- pidns_test: listed PID namespace tests passed
- test_sysinfo: PASS
- usertests -q: ALL TESTS PASSED

No kernel panic was found in the reviewed test output.

## 8. Test Evidence

- docs/test_evidence/scrum94_memory_quota.log
- docs/test_evidence/scrum94_regression.log

## 9. Known Limitations

- Quota includes process-image physical pages, not
  all kernel memory overhead.

- Exec temporarily charges both old and new images.
  A replacement may therefore fail under a tight quota.

- Physical out-of-memory fault injection during
  partial allocation has not been independently tested.

- Partial exec allocation rollback requires additional
  dedicated testing.

- Sparse address-space scanning may add overhead
  for large lazy virtual-memory reservations.

- Container destruction and ID reuse are outside
  the scope of SCRUM-94.

## 10. Conclusion

Physical memory quota enforcement has been
implemented and verified in the tested scenarios.

Memory accounting increases during physical allocation
and decreases during deallocation and process reaping.

The memory quota tests and xv6 usertests regression
completed successfully.

Remaining limitations are documented for future work.
