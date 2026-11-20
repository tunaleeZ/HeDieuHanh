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

## Tuần 4 – Tổng hợp lỗi và hướng khắc phục

### F1 – Container chưa cấu hình root

Trạng thái: Đã sửa và kiểm thử lại thành công.

Container chưa có root hợp lệ không được phép
thực hiện cont_enter().

### F2 – Bỏ sót thời gian CPU dưới một tick

Trạng thái: Đã cải tiến và kiểm chứng.

Thay thế cơ chế đếm timer interrupt bằng đo runtime
thông qua bộ đếm thời gian RISC-V r_time().

Kết quả kiểm thử ghi nhận runtime từ 45166
đến 344102 đơn vị thời gian trong các mẫu
có cpu_used_ticks bằng 0.

### F3 – Throttle tiến trình giữ sleeplock

Trạng thái: Đã kiểm thử, chưa giải quyết hoàn toàn.

Hai tiến trình host và container cùng ghi file
và đều hoàn thành 100 vòng ghi.

Host hoàn thành trong 25 ticks.
Container hoàn thành trong 379 ticks.

Không quan sát thấy deadlock trong lần kiểm thử.

Tuy nhiên, chưa chứng minh an toàn trong mọi
trường hợp tranh chấp sleeplock.

### F4 – Tràn số nguyên khi reserve memory quota

Trạng thái: Đã sửa.

Bổ sung kiểm tra giới hạn trước phép cộng
trong container_reserve_pages().

Các bài kiểm thử memory quota đã thành công.

### F5 – Sai lệch mốc thời gian CPU period

Trạng thái: Đã cải tiến.

Bổ sung kiểm tra mẫu thời gian cũ nhằm ngăn
unsigned underflow.

Sau đó chuyển cơ chế period refresh sang r_time().

Chưa có bài kiểm thử tái hiện race condition riêng.

### F10 – Host container root

Trạng thái: Đã sửa và regression thành công.

Không cho phép cấu hình root của container 0.

### F11 – Hủy và tái sử dụng container

Trạng thái: Chưa triển khai.

Container không được tái sử dụng hoàn chỉnh
sau khi namespace-init kết thúc.

Việc chạy nhiều bài kiểm thử trong cùng phiên
QEMU có thể làm cạn slot container.

### DEFECT-01 – Nhập nhầm lệnh trong QEMU

Một số lệnh Ubuntu được nhập vào shell xv6,
gây thông báo exec failed.

Host vẫn hoạt động trong các phiên kiểm thử.

Nguyên nhân của hiện tượng console ban đầu
chưa được xác nhận hoàn toàn.

### DEFECT-02 – Hết dung lượng filesystem

Hiện tượng:

    test writebig: balloc: out of blocks
    SOME TESTS FAILED

Nguyên nhân có khả năng cao:
Filesystem không đủ block trống sau khi bổ sung
nhiều chương trình kiểm thử.

Giải pháp:

- Tăng FSSIZE từ 2000 lên 4000 blocks.
- Clean build.
- Tạo lại filesystem image.
- Chạy lại usertests -q.

Kết quả:

    ALL TESTS PASSED
    HOST_OK

Commit khắc phục: 4de9a01.

Bằng chứng:

- docs/test_evidence/scrum97_usertests_fsfull.log
- docs/test_evidence/scrum97_final_usertests.log

### DEFECT-03 – Vượt CPU quota trên nhiều hart

Trạng thái: Hạn chế đã được ghi nhận.

Ngân sách CPU kiểm thử: 5000000 đơn vị runtime.

Runtime lớn nhất quan sát được: 7559548.

Mức vượt ngân sách tại mẫu quan sát: 51,19%.

Nguyên nhân phù hợp với cơ chế soft quota:
Nhiều hart có thể thực thi đồng thời trước
khi runtime được cập nhật.

Cần nghiên cứu cơ chế giới hạn chặt hơn
trong các tuần tiếp theo.

### Hạn chế cách ly file descriptor

File descriptor đã được mở trước khi chroot
vẫn có thể sử dụng sau khi thay đổi root.

Cơ chế hiện tại chủ yếu cách ly đường dẫn,
không cung cấp cách ly tài nguyên tuyệt đối.
