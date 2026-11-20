# Kiến trúc Lightweight Containers – Tuần 4

## 1. Mục tiêu

Xây dựng cơ chế container nhẹ trên xv6-riscv
nhằm cách ly tiến trình, thư mục gốc và giới hạn
tài nguyên sử dụng.

## 2. Kiến trúc tổng thể

Hệ thống gồm các thành phần:

- Giao diện user-space.
- Giao diện system call.
- Bộ quản lý container.
- Bộ quản lý tiến trình.
- PID namespace.
- Bộ quản lý bộ nhớ.
- Bộ lập lịch CPU.
- Hệ thống tập tin.

Các container sử dụng chung kernel xv6.

## 3. Giao diện system call

- cont_create(): Tạo container.
- cont_enter(): Đưa tiến trình vào container.
- cont_info(): Truy vấn trạng thái container.
- cont_setroot(): Cấu hình thư mục gốc.
- cont_set_mem(): Cấu hình giới hạn RAM.
- cont_set_cpu(): Cấu hình giới hạn CPU.

## 4. Quản lý tiến trình

Mỗi container quản lý số lượng tiến trình
thông qua proc_count và max_proc.

Yêu cầu fork bị từ chối khi vượt giới hạn.

Slot tiến trình được thu hồi khi wait()
hoàn thành quá trình reap.

## 5. PID namespace

Mỗi container có không gian PID riêng.

Tiến trình đầu tiên có virtual PID bằng 1.

Kernel sử dụng PID toàn cục để quản lý
tiến trình ở mức hệ thống.

## 6. Quản lý bộ nhớ

Bộ đếm pages_used phản ánh số trang vật lý
thuộc image tiến trình trong container.

Giới hạn được kiểm tra trước khi cấp phát.

Hỗ trợ eager allocation, lazy allocation,
fork, exec và thu hồi bộ nhớ.

## 7. Quản lý CPU

CPU runtime được đo bằng r_time().

Runtime được cộng dồn giữa các lần context switch.

Container hết ngân sách sẽ bị scheduler
tạm thời bỏ qua cho đến chu kỳ tiếp theo.

Timer interrupt vẫn thực hiện chức năng preemption.

Host container không bị throttle.

## 8. Cách ly hệ thống tập tin

Mỗi container có thư mục root riêng.

Các đường dẫn được phân giải trong giới hạn root.

Tiến trình con kế thừa root của tiến trình cha.

Container chưa cấu hình root không được phép enter.

## 9. Đồng bộ và khóa

Các bộ đếm tài nguyên được bảo vệ bằng
container spinlock.

Không giữ container lock qua swtch().

Thứ tự khóa liên quan:

    wait_lock -> p->lock -> container->lock

## 10. Môi trường thực thi

- QEMU RISC-V.
- 3 hart.
- RAM: 128 MiB.
- Filesystem: 4000 blocks sau điều chỉnh.

## 11. Giới hạn của kiến trúc

- Chưa hỗ trợ strict CPU quota.
- Chưa loại bỏ hoàn toàn rủi ro sleeplock.
- Chưa hỗ trợ destroy/reuse container.
- File descriptor kế thừa chưa được cách ly tuyệt đối.
- Memory quota không tính toàn bộ kernel overhead.
- Container-run chưa được xác nhận tích hợp.

## 12. Kết luận

Kiến trúc tuần 4 đã tích hợp các chức năng chính
của Lightweight Containers và có bằng chứng
kiểm thử cho phần lớn yêu cầu kỹ thuật.

Các hạn chế được ghi nhận để tiếp tục phát triển
trong những tuần tiếp theo.
