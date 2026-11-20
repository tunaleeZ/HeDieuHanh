# SCRUM-97 – Bảng kiểm thử tích hợp tuần 4

## 1. Thông tin chung

- Đề tài: Lightweight Containers trên xv6-riscv.
- Nhánh phát triển: main.
- Hệ điều hành phát triển: Ubuntu.
- Môi trường kiểm thử: QEMU RISC-V.
- Bộ nhớ RAM: 128 MiB.
- Số hart: 3.
- Kích thước trang bộ nhớ: 4096 byte.
- Kích thước filesystem sau điều chỉnh: 4000 blocks.

## 2. Quy ước đánh giá

- PASS: Đạt yêu cầu trong phạm vi đã kiểm thử.
- PARTIAL PASS: Đạt một phần yêu cầu, còn hạn chế.
- FAIL: Đã thực hiện kiểm thử nhưng không đạt.
- BLOCKED: Chưa thể hoàn thành do thiếu chức năng.
- NOT RUN: Chưa thực hiện kiểm thử.

## 3. Kết quả kiểm thử

| Mã | Nội dung kiểm thử | Kết quả | Bằng chứng |
|---|---|---|---|
| T01 | Khởi động hệ điều hành | PASS | scrum97_final_build.log |
| T02 | Thực thi chương trình user | PASS | scrum97_final_build.log |
| T03 | Cách ly không gian PID | PASS | scrum97_final_cpu_pid.log |
| T04 | Giới hạn số lượng tiến trình | PASS | scrum97_final_build.log |
| T05 | Thu hồi quota tiến trình | PASS | scrum97_final_build.log |
| T06 | Ngăn chặn fork bomb có giới hạn | PASS | scrum97_T06_forkbomb.log |
| T07 | Cấp phát RAM dưới giới hạn | PASS | scrum94_memory_quota.log |
| T08 | Từ chối cấp phát RAM vượt giới hạn | PASS | scrum94_memory_quota.log |
| T09 | Thu hồi bộ nhớ và rollback | PASS | scrum94_memory_quota.log |
| T10 | Giới hạn thời gian CPU | PASS | scrum97_final_cpu_pid.log |
| T11 | Làm mới quota CPU theo chu kỳ | PASS | scrum95_cpu_runtime.log |
| T12 | An toàn CPU trên nhiều hart | PARTIAL PASS | scrum95_subtick.log; scrum95_lock_test.log |
| T13 | Ngăn thoát khỏi thư mục gốc | PASS | scrum97_T13_root.log |
| T14 | Cách ly nhiều container đồng thời | PASS | scrum97_T14_multi.log |
| T15 | Hủy và tái sử dụng container | BLOCKED | Chưa triển khai |
| T16 | Kiểm thử hồi quy xv6 | PASS | scrum97_final_usertests.log |

Các file bằng chứng nằm trong docs/test_evidence/.

## 4. Kết quả nổi bật

### T06 – Ngăn chặn fork bomb

- Giới hạn: 8 tiến trình.
- Số lần fork thành công: 7.
- Số lần fork bị từ chối: 9.
- Số tiến trình lớn nhất quan sát được: 8.
- Số trang bộ nhớ lớn nhất quan sát được: 32.
- Host vẫn có tiến triển trong quá trình kiểm thử.
- Tài nguyên được thu hồi sau khi tiến trình kết thúc.
- Kết quả: 0 failures.

### T07–T09 – Giới hạn bộ nhớ

- Giới hạn kiểm thử: 32 trang.
- Bộ nhớ ban đầu: 5 trang.
- Sau cấp phát eager: 9 trang.
- Sau thu hồi: 5 trang.
- Lazy allocation chỉ tính các trang đã truy cập.
- Fork, exec và cấp phát vượt giới hạn bị từ chối.
- Bộ nhớ trở về 0 sau khi container kết thúc.
- Kết quả: 0 failures.

### T10–T12 – Giới hạn CPU

- Thay thế cơ chế đếm timer interrupt bằng runtime accounting.
- Ghi nhận được thời gian thực thi nhỏ hơn một CPU-tick.
- Kiểm thử CPU quota thành công.
- Kiểm thử nhiều tiến trình dùng chung quota thành công.
- Hai container có quota riêng biệt hoạt động bình thường.
- Vẫn tồn tại hiện tượng vượt quota tạm thời trên nhiều hart.
- Chưa chứng minh an toàn locking trong mọi trường hợp.

### T13 – Cách ly thư mục gốc

- Chặn truy cập đường dẫn bên ngoài root.
- Chặn thoát root bằng đường dẫn cha.
- Tiến trình con kế thừa root.
- Thực thi chương trình bên trong root thành công.
- Kiểm thử quyền cấu hình container thành công.
- Root test: 0 failures.
- Permission test: 0 failures.

File descriptor được mở trước khi chroot vẫn có thể sử dụng.
Đây là giới hạn của cơ chế cách ly hiện tại.

### T14 – Nhiều container đồng thời

Container A:

- Root: /t14a.
- Giới hạn tiến trình: 4.
- Giới hạn RAM: 32 trang.
- CPU quota: 2/20 ticks.
- Số vòng lặp CPU: 31730000.

Container B:

- Root: /t14b.
- Giới hạn tiến trình: 6.
- Giới hạn RAM: 64 trang.
- CPU quota: 10/20 ticks.
- Số vòng lặp CPU: 202355000.

Hai container có PID namespace riêng, root riêng
và cấu hình giới hạn tài nguyên độc lập.

Sau kiểm thử, tài nguyên tiến trình và bộ nhớ
được thu hồi đầy đủ.

Kết quả: 0 failures.

### T16 – Kiểm thử hồi quy

Lần kiểm thử đầu tiên gặp lỗi hết block filesystem
tại bài kiểm thử writebig.

Đã tăng FSSIZE từ 2000 lên 4000 blocks,
tạo lại filesystem và kiểm thử lại.

Kết quả cuối cùng:

ALL TESTS PASSED
HOST_OK

## 5. Thống kê

- Tổng số test: 16.
- PASS: 14.
- PARTIAL PASS: 1.
- BLOCKED: 1.
- Tỷ lệ PASS: 87,5%.

Đây là kết quả kiểm thử tích lũy trong tuần 4.
Không phải toàn bộ 16 test đều được chạy lại
sau commit thay đổi dung lượng filesystem.

## 6. Hạn chế còn tồn tại

- CPU quota vẫn là soft quota.
- Có thể vượt ngân sách CPU trên nhiều hart.
- Tiến trình giữ sleeplock có thể bị throttle.
- Chưa hỗ trợ hủy và tái sử dụng container.
- Chưa thực hiện đầy đủ fault injection khi hết RAM.
- Chưa xác nhận chương trình Container-run đã được tích hợp.

## 7. Kết luận

Các chức năng quản lý tiến trình, PID namespace,
cách ly thư mục gốc, giới hạn RAM và giới hạn CPU
đã được tích hợp và kiểm thử.

Hệ thống đã đạt phần lớn mục tiêu kỹ thuật tuần 4.

Hai hạng mục T12 và T15 vẫn cần tiếp tục hoàn thiện
trong giai đoạn phát triển tiếp theo.
