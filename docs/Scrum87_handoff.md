# Scrum87 – Per-Container Measurement and Benchmark Rerun – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: hoàn tất

## 1. Mục tiêu
Xác nhận `cont_info` xuất CPU fields đúng với ABI user/kernel; sau đó chạy lại bench_cpu, bench_mem, bench_proc ít nhất năm lần mỗi loại với cấu hình cố định và lưu số liệu thật.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 6.1 Extend cont_info output with CPU fields | đã commit | 9c7cb22 | Kernel/user field order and types match; cputest reads before/during/after |
| 6.2 Repeat existing benchmarks with fixed parameters | đã commit | commit này trong git log | Năm boot sạch; ba benchmark mỗi lượt; raw logs và thống kê đã lưu |

## 3. Bằng chứng
6.1: `make fs.img` PASS; QEMU 10.2.1, 128M, 3 hart: `cputest` PASS. `cont_info` báo đúng period=20, quota=5; used=0 trước/sau và 5 trong lúc tải. Log: `docs/test_evidence/T10_cont_info_cpu.txt`.
6.2: `make clean`, `make`, `make fs.img` PASS. Five fresh QEMU boots, `-smp 3`; each ran `bench_cpu` (100 ticks), `bench_mem` (32 pages), and `bench_proc` (100 forks). All memory/process checks PASS; CPU average 6020.2 iterations/tick, population SD 137.24. Raw logs `benchmark_run_1.txt` … `benchmark_run_5.txt`; analysis `benchmark_rerun_week3.md`. Final regressions are in `final_regression_week3.txt`.

## 4. Kết luận
SCRUM-87 hoàn tất với hai commit. Lịch sử baseline `6539 iterations/tick` không có raw run parameters; mục 6.2 ghi rõ phần chênh lệch không thể quy nguyên nhân cho tham số thiếu bằng chứng. Không push.
