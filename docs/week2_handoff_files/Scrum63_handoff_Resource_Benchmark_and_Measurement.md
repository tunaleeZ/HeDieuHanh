# HANDOFF – SCRUM-63: Resource Benchmark and Measurement

**Dự án:** Lightweight Containers trên xv6-riscv  
**Môi trường:** Ubuntu, QEMU, xv6-riscv  
**Nhánh làm việc:** main  
**Trạng thái:** Các kiểm thử chức năng cơ bản đã PASS; cần hoàn tất review và commit.

## 1. Mục tiêu

Xây dựng cơ chế thu thập thông số CPU, RAM, process và chương trình benchmark phục vụ đánh giá hiệu năng của hệ thống Lightweight Containers.

## 2. Các thành phần đã triển khai

| Thành phần | File | Chức năng |
|---|---|---|
| Resource syscall | kernel/sysproc.c | `sys_sysinfo()` |
| Free-memory counter | kernel/kalloc.c | `kfreemem()` |
| Process counter | kernel/proc.c | `knproc()` |
| Shared structure | kernel/sysinfo.h | `struct sysinfo` |
| CPU Benchmark | user/bench_cpu.c | Đo số vòng lặp theo tick |
| Memory Benchmark | user/bench_mem.c | Đo cấp phát và thu hồi RAM |
| Process Benchmark | user/bench_proc.c | Đo fork/wait tuần tự |
| Syscall test | user/test_sysinfo.c | Kiểm thử dữ liệu syscall |

Các file đăng ký syscall và Makefile cũng đã được cập nhật.

## 3. Kết quả kiểm thử

| Kiểm thử | Kết quả | Trạng thái |
|---|---|---|
| `test_sysinfo` | Free RAM 133255168 bytes, 3 process slots | PASS |
| `bench_cpu` | 230409 iterations / 100 ticks | PASS |
| `bench_mem` | Cấp phát và thu hồi 131072 bytes | PASS |
| `bench_proc` | 100/100 fork thành công, 2 ticks | PASS |

Các kết quả trên thuộc những lần chạy được ghi nhận, không phải giá trị trung bình nhiều lần.

## 4. Hướng dẫn chạy lại

Trong Ubuntu:

1. Vào thư mục repository.
2. Biên dịch và khởi động bằng `make qemu`.
3. Trong shell xv6, chạy riêng từng chương trình: `test_sysinfo`, `bench_cpu`, `bench_mem`, `bench_proc`.
4. Ghi lại kết quả và so sánh với baseline.

## 5. Điểm cần lưu ý

- `sysinfo()` đang cung cấp thống kê toàn hệ thống, chưa tách riêng từng container.
- `uptime()` đo thời gian theo tick, không phản ánh chính xác CPU time của tiến trình.
- `bench_proc` thực hiện fork/wait tuần tự.
- Chưa có kết quả stress test hoặc concurrency benchmark.
- Kết quả benchmark cần được lặp lại nhiều lần nếu dùng để so sánh hiệu năng.

## 6. Liên kết các công việc

**SCRUM-61:** Cung cấp process tracking và process quota.

**SCRUM-62:** Cung cấp cơ chế chroot và path-boundary isolation.

**SCRUM-64:** Có thể sử dụng dữ liệu benchmark và memory accounting để đánh giá kiến trúc, thiết kế CPU quota.

**SCRUM-65:** Sử dụng các chương trình benchmark trong regression testing.

## 7. Bàn giao

Các thành phần chính đã chạy thành công trong môi trường xv6-riscv trên QEMU.

Việc cần thực hiện trước khi đóng task:

- Review toàn bộ thay đổi code.
- Kiểm tra trạng thái Git.
- Chạy lại build và kiểm thử.
- Commit các file
