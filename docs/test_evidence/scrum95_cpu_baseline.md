# SCRUM-95 - CPU Quota Baseline

## Test Environment

- QEMU RISC-V
- 3 harts
- CPU quota: 5 CPU ticks
- CPU period: 20 wall-clock ticks
- Test duration: 200 ticks
- Number of runs: 3

## Results

| Run | Container A | Host B | A/B ratio | Zero windows |
|-----|-------------|--------|-----------|--------------|
| 1 | 100529000 | 472103000 | 21.29% | 0 |
| 2 | 82208000 | 390690000 | 21.04% | 0 |
| 3 | 78365000 | 311620000 | 25.15% | 0 |

Mean A/B ratio: 22.49%.

Nominal CPU budget: 25% of one CPU.

## Verification

- CPU quota test: PASS (3/3 runs).
- CPU period progress: PASS in measured windows.
- Host shell responsiveness: HOST_OK.
- No kernel panic reported in the reviewed output.

## Limitations

Iteration ratios are indirect performance indicators,
not direct measurements of CPU execution time.

The tests do not independently reproduce the stale
time-sample race described in F5.

Sub-tick execution accounting, concurrent workers
within one container and sleep-lock contention require
additional dedicated tests.

## Evidence

docs/test_evidence/scrum95_cpu_baseline.log
