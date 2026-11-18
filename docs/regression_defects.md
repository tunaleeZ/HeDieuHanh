# SCRUM-65 - Regression Defects

## DEFECT-01: Console input issue

Status: Under investigation
Severity: Low (provisional)

### Observation
During QEMU testing, the xv6 console displayed:
- exec xv6 failed
- exec init: failed

Some boot messages appeared again at the shell prompt.

### Impact
The shell was still responsive.

Verified commands:
- ls: PASS
- echo OK: PASS
- ctest: PASS

### Possible cause
Console input may have contained pasted boot messages.
Root cause has not been confirmed.

### Next steps
1. Restart QEMU with a clean terminal session.
2. Run one command at a time.
3. Check whether the issue occurs without user input.
4. Record the result.

## Regression summary

- Boot and basic shell: PASS
- Container process quota: PASS
- Remaining regression tests: NOT RUN in latest session

No additional defects have been confirmed.
## Week 3 - DEFECT-01 Retest (2026-11-11)

### Test environment
- OS: Ubuntu
- Emulator: QEMU
- Target: xv6-riscv
- CPU: 3 harts
- Branch: main

### Regression test

Command: usertests -q

Result: PASS

Output:
- usertests starting
- ALL TESTS PASSED

### DEFECT-01 verification

Searched for:
- exec xv6 failed
- exec init: failed

Result:
- Neither error was found in the new regression log.
- No kernel panic was found in the checked output.
- Shell remained operational during the test.

### Conclusion

Status: NOT REPRODUCED IN THIS SESSION

The console input issue was not reproduced in the
2026-11-11 regression session.

Root cause remains unconfirmed.
Do not mark this defect as permanently resolved.

### Evidence

docs/test_evidence/scrum88_full_regression.log

## SCRUM92 — F1/F10 disposition and DEFECT-01 retest (2026-11-18)

- F1 root-unset isolation: fixed in `kernel/proc.c`. `cont_enter()` now rejects
  a target with no root before reserving a process slot; `ptest_neg` confirms
  the return value and unchanged `proc_count`. `/c1` isolation, absolute and
  relative outside-path denial, and cwd reset pass in `roottest`.
- F10: policy now rejects `cont_setroot(0, "/")`; the valid-path negative case
  passes in `ptest_neg`.
- DEFECT-01 retest: `usertests -q` completed with `ALL TESTS PASSED`; shell
  commands were run individually and neither `exec xv6 failed` nor
  `exec init: failed` appeared in the captured transcript. The defect was not
  reproduced; its root cause remains unconfirmed and status remains under
  investigation.
- Evidence: `docs/test_evidence/T13_f1_enforcement.txt` and
  `docs/test_evidence/F1_regression.txt`.
