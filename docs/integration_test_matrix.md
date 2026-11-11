# SCRUM-88 - Integration Test Matrix

Project: Lightweight Containers on xv6-riscv
Branch: main
Environment: QEMU, 128 MiB, 3 harts

Test plan: docs/week1/test_plan_vi_khong_bang.md

Status definitions:
- PASS: Supported by actual test evidence in stated scope.
- FAIL: Test ran and failed.
- BLOCKED: Required feature is not implemented.
- NOT RUN: Full test scenario has not been verified.

| ID | Test case | Status | Evidence / Notes |
|---|---|---|---|
| T01 | Basic boot | PASS | scrum89_clean_build.log; clean build and boot |
| T02 | User programs | PASS | scrum89_clean_build.log; ls, forktest, ctest |
| T03 | Independent PID namespace | BLOCKED | SCRUM-90 design only; PID namespace not implemented |
| T04 | Process limit | PASS | scrum89_clean_build.log; ctest max_proc=3, overflow rejected; plan suggests 4 |
| T05 | Process quota release | PASS | scrum89_clean_build.log; wait and fork again succeeded |
| T06 | Fork bomb containment | NOT RUN | Bounded recursive fork stress and host-shell responsiveness not verified together |
| T07 | Memory below quota | BLOCKED | T07_mem_config.txt proves configuration only; allocation enforcement not connected |
| T08 | Memory above quota | BLOCKED | VM allocation enforcement not implemented in available evidence |
| T09 | Memory release and fork | BLOCKED | Per-container page accounting/recovery not completed |
| T10 | CPU quota | PASS | T10_cpu_quota.txt; 3 runs PASS; approximate iterations-based result |
| T11 | CPU quota refresh | PASS | T11_cpu_refresh.txt; 30 nonzero windows across 3 runs |
| T12 | Multi-hart CPU safety | NOT RUN | QEMU 3 harts; no full concurrency/race safety test |
| T13 | Root escape protection | PASS | T13_root_escape.txt; roottest 0 failures |
| T14 | Multiple concurrent containers | NOT RUN | Different roots/quotas running concurrently not fully verified |
| T15 | Container cleanup and reuse | BLOCKED | Container destroy/reuse lifecycle not implemented |
| T16 | xv6 regression | PASS | scrum88_full_regression.log; usertests -q ALL TESTS PASSED |

## Additional verification

- ptest_neg: PASS
  Evidence: T13_permission_negative.txt
- chroottest /c1: PASS
  Evidence: final_regression_week3.txt
- test_sysinfo: PASS
  Evidence: final_regression_week3.txt
- CPU benchmark reruns:
  Evidence: benchmark_rerun_week3.md
- DEFECT-01: Not reproduced in the latest usertests session.
  Root cause not confirmed.
  Evidence: scrum88_full_regression.log

## Outstanding work

1. Run bounded recursive fork stress for T06.
2. Implement and verify PID namespace for T03.
3. Implement memory quota enforcement and recovery for T07-T09.
4. Run comprehensive multi-hart concurrency tests for T12.
5. Run simultaneous-container isolation tests for T14.
6. Implement container cleanup/reuse for T15.

## Conclusion

Basic process quota, CPU quota prototype, root isolation,
permission checks, and xv6 regression have passing evidence.

The full 16-case acceptance suite is not yet complete.
