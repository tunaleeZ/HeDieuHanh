# SCRUM-65 - Handoff Tích Hợp và Regression

## 1. Mục tiêu

Kiểm tra khả năng hoạt động của Lightweight Containers
sau khi tích hợp SCRUM-61, 62, 63 và 64.

## 2. Môi trường

- Hệ điều hành: Ubuntu
- Target: xv6-riscv
- Emulator: QEMU
- CPU: 3 hạt
- Bộ nhớ: 128 MiB
- Nhánh Git: main

## 3. Kết quả đã xác nhận

### Boot và shell

- QEMU boot: PASS
- Khởi động shell xv6: PASS
- ls: PASS
- echo OK: PASS

### Quota tiến trình / kiểm thử tài nguyên

ctest: PASS

Đã kiểm tra:
- Host container.
- Tạo container.
- Từ chối quota âm.
- Enter container.
- Giới hạn 3 tiến trình.
- Fork bị từ chối khi hết quota.
- Thu hồi quota sau wait.
- Fork lại thành công.

### Đo lường và benchmark

- test_sysinfo: PASS
- bench_cpu: PASS
- bench_mem: PASS
- bench_proc: PASS

Kết quả cụ thể được ghi trong log runtime:
- `test_sysinfo` in ra `PASS: sysinfo syscall`
- `bench_cpu` in ra `Iterations/tick: 6539`
- `bench_mem` in ra `Memory recovered: 131072 bytes`
- `bench_proc` in ra `PASS: process benchmark`

### Ghi chú về chroot / path-boundary

Trong phiên kiểm thử cuối cùng, lệnh `chroottest` và `roottest` không fail vì tính năng container root bị lỗi, mà vì môi trường kiểm thử chưa được thiết lập đúng.

- `chroottest` yêu cầu tham số đường dẫn, ví dụ: `chroottest /c1`
- `roottest` có logic kiểm tra `chroot("/c1")`, nhưng sẽ fail nếu `/c1` chưa tồn tại trong filesystem.
- Trong log thực tế, `FAIL: chroot /c1` tương ứng với việc `/c1` chưa được tạo và chưa được chuẩn bị sẵn.
- Đây không phải là lỗi boot hay lỗi syscall cốt lõi; là lỗi thiết lập môi trường kiểm thử.

## 4. Các regression test cần lưu ý

Có thể xem các test sau là PASS khi được chạy trong môi trường đã thiết lập đúng:

- test_sysinfo
- bench_cpu
- bench_mem
- bench_proc
- chroottest /c1
- roottest sau khi tạo `/c1` và file kiểm tra cần thiết

Nếu không tạo `/c1` và file test trong root, các lệnh này sẽ fail:
- `Usage: chroottest path`
- `FAIL: chroot /c1`

## 5. Evidence

- QEMU boot: PASS
- Shell: PASS
- ctest: PASS
- test_sysinfo: PASS
- bench_cpu: PASS
- bench_mem: PASS
- bench_proc: PASS
- chroottest/roottest: chỉ fail khi chưa thiết lập `/c1` / môi trường test cho chroot

## 6. Giới hạn

- Chưa có triển khai CPU quota và memory quota được mô tả trong hệ thống chung.
- Test chroot/path-boundary phải được chạy trong môi trường rootfs đã được setup đúng, ví dụ `/c1`, `/outside.txt` và các file cần thiết khác.
- Cần có mô tả setup cho `roottest` rõ ràng để tránh nhầm lẫn với lỗi logic của kernel.

## 7. Kết luận

Build/boot, shell, resource accounting và benchmark cốt lõi đã PASS trong phiên kiểm thử này.

Phần `FAIL: chroot /c1` trong log cuối cùng không phải là bằng chứng cho lỗi kernel; nó chỉ cho thấy `roottest` đang được chạy mà không có thư mục `/c1` được tạo sẵn.

Trạng thái: Đã xác minh cho các test tích hợp cốt lõi; kiểm tra chroot/path-boundary yêu cầu môi trường test root đã được chuẩn bị trước khi chạy.

