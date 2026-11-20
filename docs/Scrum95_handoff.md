# SCRUM-95 - CPU Quota Safety and Locking

## 1. Objective

Improve container CPU quota accounting, period refresh,
multi-hart behavior and locking safety in xv6-riscv.

## 2. F5 - Period Refresh

Added protection against stale time samples that could
cause unsigned subtraction underflow.

The runtime implementation subsequently migrated
period refresh to the RISC-V time counter.

## 3. F2 - Runtime Accounting

Replaced interrupt-count accounting with runtime-based
accounting around scheduler context switches.

Runtime is accumulated in RISC-V time-counter units.

One nominal CPU tick corresponds to 1000000 units.

Added runtime monitoring through cont_info().

Sub-tick verification:

- cpu_used_ticks remained zero.
- cpu_used_runtime reported positive values.
- Observed nonzero samples: 45166 to 344102 units.

Result: Sub-tick accounting behavior verified.

## 4. CPU Baseline Comparison

Timer accounting:

- Run 1: 21.29%
- Run 2: 21.04%
- Run 3: 25.15%
- Mean iteration ratio: approximately 22.49%

Runtime accounting:

- Run 1: 30.01%
- Run 2: 23.59%
- Run 3: 22.54%
- Mean iteration ratio: approximately 25.38%

Nominal budget: 25% of one CPU.

All six cputest runs passed.

Iteration ratios are indirect performance indicators,
not direct measurements of CPU execution time.

## 5. Multi-hart Testing

Environment: QEMU with three harts.

Three workers shared the CPU quota of one container.

Limited workload: 137090000 iterations.
Unlimited workload: 1456345000 iterations.

Relative throughput: approximately 9.41%.

CPU safety test: 0 failures.

Maximum observed runtime: 7559548 units.
Configured runtime budget: 5000000 units.

Observed overshoot: approximately 51.19%.

The design remains a soft quota implementation.

## 6. F3 - Filesystem Contention

Container worker:
- Quota: 1 tick per 20 ticks.
- Completed 100 file-write rounds.
- Elapsed: 379 ticks.

Host worker:
- Completed 100 file-write rounds.
- Elapsed: 25 ticks.

CPU lock test: 0 failures.
Host responsiveness: HOST_OK.

No deadlock was observed in this workload.

The test does not prove that throttling a sleeplock
holder is safe in every possible execution.

## 7. Regression Testing

- Memory quota test: 0 failures.
- Container test: 0 failures.
- Initial PID namespace scenarios: PASS.
- Sysinfo syscall: PASS.
- xv6 usertests: ALL TESTS PASSED.
- Host responsiveness: HOST_OK.

A repeated PID namespace setup failed in the same
QEMU session. Container slot exhaustion is suspected.
A fresh-session test is required to confirm behavior.

## 8. Known Limitations

- CPU enforcement remains a soft quota.
- Concurrent running processes may overshoot the budget.
- Runtime monitoring reports completed accounting slices.
- Sleeplock-holder throttling remains a potential risk.
- Container destruction and slot reuse are unsupported.
- Kernel timing and context-switch overhead introduce
  measurement error.

## 9. Test Evidence

- docs/test_evidence/scrum95_cpu_baseline.log
- docs/test_evidence/scrum95_cpu_runtime.log
- docs/test_evidence/scrum95_cpu_safety.log
- docs/test_evidence/scrum95_subtick.log
- docs/test_evidence/scrum95_lock_test.log
- docs/test_evidence/scrum95_regression.log

## 10. Conclusion

Runtime-based CPU quota accounting has been implemented.

Sub-tick runtime recording, CPU throttling,
multi-worker execution and filesystem contention
have been verified in the tested scenarios.

Remaining soft-quota and locking limitations are
documented and require further improvement.
