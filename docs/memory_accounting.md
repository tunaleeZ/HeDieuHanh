# SCRUM-63.6 – Memory Accounting Analysis

## 1. Mục tiêu

Phân tích cơ chế quản lý bộ nhớ trong xv6-riscv, xác định các điểm cấp phát và thu hồi tài nguyên, đồng thời xây dựng phương pháp đo bộ nhớ vật lý phục vụ Resource Benchmark.

## 2. Cơ chế quản lý bộ nhớ vật lý

Bộ cấp phát bộ nhớ trong `kernel/kalloc.c` sử dụng danh sách liên kết `kmem.freelist`. Mỗi phần tử biểu diễn một trang vật lý có kích thước 4096 byte.

Các hàm chính:

- `kinit()`: Khởi tạo bộ cấp phát bộ nhớ.
- `freerange()`: Đưa các trang vật lý hợp lệ vào danh sách trống.
- `kalloc()`: Lấy một trang từ danh sách trống.
- `kfree()`: Trả một trang về danh sách trống.
- `kfreemem()`: Đếm số trang trống và quy đổi thành số byte RAM trống.

Hàm `kfreemem()` được bổ sung trong SCRUM-63, sử dụng `kmem.lock` để bảo vệ danh sách trong quá trình duyệt.

Công thức:

**Free Memory = Free Pages × 4096 bytes**

Giá trị đo được phản ánh số byte trong danh sách trang trống của bộ cấp phát xv6, không phải tổng bộ nhớ khả dụng theo hệ thống Linux.

## 3. Cơ chế cấp phát bộ nhớ ảo

Phân tích `kernel/vm.c`:

- `uvmalloc()`: Cấp phát và ánh xạ các trang bộ nhớ vật lý.
- `uvmdealloc()`: Thu nhỏ không gian địa chỉ và thu hồi các trang không còn sử dụng.
- `uvmfree()`: Giải phóng bộ nhớ user-space và page table.
- `uvmcopy()`: Sao chép các trang đã ánh xạ từ tiến trình cha sang tiến trình con.
- `vmfault()`: Xử lý truy cập vào trang chưa ánh xạ, hỗ trợ lazy allocation.

Với lazy allocation, việc tăng kích thước vùng nhớ qua `sbrk()` không nhất thiết làm giảm ngay số trang RAM trống. Trang vật lý được cấp phát khi tiến trình thực sự truy cập vùng nhớ tương ứng.

## 4. Quản lý tài nguyên tiến trình

Trong `kernel/proc.c`:

- `kfork()`: Kiểm tra quota container, cấp phát tiến trình và sao chép vùng nhớ.
- `freeproc()`: Giải phóng các tài nguyên liên quan đến tiến trình.
- `kwait()`: Thu hồi tiến trình con đã kết thúc và gọi `freeproc()`.
- `knproc()`: Đếm số slot tiến trình có trạng thái khác `UNUSED`.

Tiến trình ở trạng thái `ZOMBIE` vẫn chiếm slot cho tới khi được thu hồi.

`knproc()` duyệt từng slot dưới spinlock riêng của tiến trình, nên kết quả không phải snapshot nguyên tử của toàn bộ bảng tiến trình.

## 5. System call đo tài nguyên

SCRUM-63 bổ sung syscall `sysinfo()` với hai trường:

- `freemem`: Số byte bộ nhớ vật lý trống.
- `nproc`: Số slot tiến trình đang sử dụng.

Syscall được đăng ký với số hiệu 28 trong phiên bản đã kiểm thử.

Dữ liệu được truyền từ kernel về user-space thông qua `copyout()`.

## 6. Kết quả thực nghiệm

### Memory Benchmark

Thử nghiệm cấp phát 32 trang, mỗi trang 4096 byte.

| Chỉ số | Kết quả |
|---|---:|
| RAM trống trước cấp phát | 133255168 bytes |
| RAM trống sau cấp phát | 133124096 bytes |
| RAM trống sau thu hồi | 133255168 bytes |
| Bộ nhớ cấp phát | 131072 bytes |
| Bộ nhớ thu hồi | 131072 bytes |
| Chênh lệch cuối bài đo | 0 bytes |

**Kết luận:** Trong lần kiểm thử này, 32 trang được cấp phát và thu hồi đầy đủ theo số liệu `sysinfo()`.

### Process Benchmark

| Chỉ số | Kết quả |
|---|---:|
| Fork attempts | 100 |
| Successful forks | 100 |
| Failed forks | 0 |
| Elapsed ticks | 2 |
| Process slots trước/sau | 3 / 3 |
| RAM trống trước/sau | 133255168 / 133255168 bytes
