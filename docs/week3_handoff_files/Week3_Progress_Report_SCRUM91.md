# Week 3 Progress Report
## Lightweight Containers on xv6-riscv

Date: 2026-11-11
Project: Lightweight Containers
Branch: main
Sprint: Week 3

## 1. Summary of Latest Progress

The team integrated container-level root management,
host-only container administration, a CPU quota prototype,
memory quota configuration, and extended resource reporting.

The xv6 kernel builds and boots under QEMU with 3 harts.
Core tests, including forktest, ctest, ptest_neg,
roottest, cputest and usertests -q, have passing evidence.

SCRUM-88 recorded regression results for T01-T16.
SCRUM-89 integrated the shared skeleton and clean-build
evidence.
SCRUM-90 produced a PID namespace design.

Some acceptance tests remain blocked or unverified.
The root-unset container entry issue remains a blocker.

## 2. Review of Technical Activities

### Process management

Container membership, process quotas and quota release
after wait have been implemented and tested.

### Filesystem isolation

Container-level root and cont_setroot are implemented.
roottest verifies path containment when root is configured.

Important limitation:
cont_enter into a container without a configured root
may retain host root/cwd. This issue remains open.

### CPU quota

CPU quota configuration, timer-based accounting,
period reset and scheduler throttling are implemented.

The tests demonstrate reduced CPU work in the
quota-bound container while the host continues running.

Limitations include timer resolution, short execution
intervals and possible throttling during sleeplock usage.

### Memory quota

cont_set_mem, quota configuration, and page
reserve/release helpers are implemented.

Memory allocation/free hooks are not fully connected.
Therefore, memory quota enforcement is not yet verified.

### Regression

forktest: PASS
ctest: PASS
ptest_neg: PASS
roottest with configured root: PASS
usertests -q: ALL TESTS PASSED

The complete T01-T16 matrix records both passing cases
and cases that remain blocked or unverified.

## 3. Personal Contribution

- Reviewed integration status of SCRUM-82 to SCRUM-87.
- Verified shared container structure and syscalls 29-31.
- Performed a clean QEMU build and boot.
- Ran forktest and ctest.
- Ran usertests -q and recorded successful completion.
- Investigated DEFECT-01 in a new test session.
- Updated the integration matrix T01-T16.
- Prepared SCRUM-89 task-to-commit traceability.
- Prepared PID namespace design (SCRUM-90).
- Consolidated week 3 architecture and progress evidence.

## 4. Developed Code Review

Reviewed components:

kernel/proc.h:
Container structure, root, CPU and memory quota fields.

kernel/proc.c:
Container management, CPU accounting, scheduler checks,
quota configuration and root assignment.

kernel/trap.c:
Timer interrupt and CPU usage accounting.

kernel/syscall.c, kernel/syscall.h:
Registration of container syscalls.

user/user.h, user/usys.pl:
User-level syscall declarations and stubs.

user/ctest.c, user/ptest_neg.c, user/cputest.c,
user/roottest.c:
Functional and negative testing.

The team review reported the following open findings:

F1: Root-unset container entry may expose host paths.
F2: CPU accounting misses sub-tick execution.
F3: Throttling may delay a process holding a sleeplock.
F4: Possible integer overflow in memory reserve.

These findings require follow-up and are not claimed fixed.

## 5. TODO

1. Resolve F1 root-unset isolation blocker.
2. Investigate CPU accounting precision and lock behavior.
3. Verify memory quota enforcement after VM hook integration.
4. Implement and test PID namespace in a future sprint.
5. Complete remaining T01-T16 blocked/not-run cases.
6. Verify container lifecycle and multi-container isolation.
7. Keep architecture documents and Jira evidence updated.

## GitHub Evidence

SCRUM-88 regression:
- 38cdb56
- 961eb6f

SCRUM-89 integration:
- e06c75f
- bcb7a93

SCRUM-90 PID namespace design:
- 52c0a09

Team review:
- 1bb5ab6
- 577eda7

Main evidence files:
- docs/week3_handoff_files/Review_week3_SCRUM82_87.md
- docs/integration_test_matrix.md
- docs/regression_defects.md
- docs/test_evidence/scrum89_clean_build.log
- docs/test_evidence/scrum88_full_regression.log
- docs/pid_namespace_design.md

## Conclusion

Week 3 integration, measurement, regression reporting,
and PID namespace design have progressed substantially.

The remaining security and resource-accounting issues
are documented for the next development cycle.
