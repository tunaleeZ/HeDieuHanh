# Scrum87 – Per-Container Measurement and Benchmark Rerun – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang thực hiện

## 1. Mục tiêu
Xác nhận `cont_info` xuất CPU fields đúng với ABI user/kernel; sau đó chạy lại bench_cpu, bench_mem, bench_proc ít nhất năm lần mỗi loại với cấu hình cố định và lưu số liệu thật.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 6.1 Extend cont_info output with CPU fields | đã commit | sẽ ghi sau | Kernel/user field order and types match; cputest reads before/during/after |
| 6.2 Repeat existing benchmarks with fixed parameters | chưa làm | — | QEMU -smp 3; mỗi lần chạy từ boot sạch |

## 3. Bằng chứng
6.1: `make fs.img` PASS; QEMU 10.2.1, 128M, 3 hart: `cputest` PASS. `cont_info` báo đúng period=20, quota=5; used=0 trước/sau và 5 trong lúc tải. Log: `docs/test_evidence/T10_cont_info_cpu.txt`.

## 4. Bước tiếp theo
Hoàn tất 6.1 rồi chạy benchmark 6.2. Không push.
