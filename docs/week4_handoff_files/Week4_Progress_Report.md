# BÁO CÁO TIẾN ĐỘ TUẦN 4

## Đề tài: Lightweight Containers trên xv6-riscv

## 1. Tổng quan tiến độ (Summary)

Trong tuần 4, nhóm tập trung hoàn thiện các chức năng
cách ly và giới hạn tài nguyên của lightweight container.

Các chức năng chính đã được tích hợp:

- Cách ly thư mục gốc.
- PID namespace.
- Giới hạn số lượng tiến trình.
- Giới hạn bộ nhớ vật lý.
- Giới hạn thời gian CPU.
- Kiểm thử nhiều container đồng thời.

Kết quả kiểm thử tổng hợp:

- 14/16 test PASS.
- 1/16 test PARTIAL PASS.
- 1/16 test BLOCKED.
- Tỷ lệ PASS: 87,5%.

## 2. Hoạt động kỹ thuật (Technical Activities)

### SCRUM-92 – Root Isolation

Kiểm tra và khắc phục lỗi cho phép container
chưa cấu hình root thực hiện cont_enter().

Kiểm tra đường dẫn tuyệt đối, tương đối,
chdir, fork, exec, link và unlink.

### SCRUM-93 – PID Namespace

Tích hợp không gian PID riêng cho container.

Kiểm thử khả năng quản lý PID, fork, wait,
kill và namespace-init.

### SCRUM-94 – Memory Quota Enforcement

Triển khai cơ chế tính số trang vật lý
được sử dụng bởi từng container.

Tích hợp giới hạn bộ nhớ vào:

- growproc().
- kfork().
- kexec().
- freeproc().
- vmfault().
- kcontainer_enter().

Kiểm thử cấp phát dưới và vượt quota,
lazy allocation và thu hồi tài nguyên.

Kết quả: 0 failures.

### SCRUM-95 – CPU Quota Safety and Locking

Thay thế cơ chế đếm timer interrupt bằng
đo thời gian thực thi thông qua r_time().

Cải thiện khả năng ghi nhận thời gian CPU
nhỏ hơn một tick.

Kiểm thử CPU quota, nhiều hart, nhiều container
và tranh chấp filesystem.

Các bài kiểm thử chức năng đã thành công.

CPU quota vẫn là soft quota và có thể
vượt ngân sách trong thời gian ngắn.

### SCRUM-96 – Container-run

Chưa xác nhận chương trình Container-run
được tích hợp đầy đủ trên nhánh main.

Cần cập nhật trạng thái từ thành viên phụ trách.

### SCRUM-97 – Integration and Regression

Tích hợp và kiểm thử các chức năng của nhóm.

Bổ sung hai chương trình:

- forkbombtest.c.
- multi_ctest.c.

Hoàn thành kiểm thử T06, T13 và T14.

Phát hiện lỗi hết block filesystem khi
chạy writebig trong usertests.

Tăng FSSIZE từ 2000 lên 4000 blocks.

Kết quả kiểm thử cuối:

ALL TESTS PASSED
HOST_OK

## 3. Đóng góp cá nhân (Personal Contribution)

Trong tuần 4, phần công việc cá nhân tập trung
vào ba nhiệm vụ SCRUM-94, SCRUM-95 và SCRUM-97.

### SCRUM-94

- Phân tích cơ chế cấp phát bộ nhớ xv6.
- Sửa lỗi tràn số nguyên memory accounting.
- Triển khai cơ chế giới hạn trang vật lý.
- Tích hợp eager và lazy allocation.
- Kiểm soát bộ nhớ khi fork, exec và enter.
- Xây dựng chương trình memquotatest.
- Thực hiện kiểm thử T07–T09.
- Viết tài liệu bàn giao.

### SCRUM-95

- Phân tích CPU quota trên nhiều hart.
- Xử lý nguy cơ unsigned underflow.
- Triển khai runtime accounting.
- Bổ sung thông số giám sát CPU runtime.
- Xây dựng chương trình kiểm thử CPU safety.
- Kiểm thử sub-tick accounting.
- Kiểm thử tranh chấp filesystem.
- Kiểm thử CPU quota độc lập.
- Viết tài liệu bàn giao.

### SCRUM-97

- Tích hợp mã nguồn của các thành viên.
- Xây dựng kiểm thử fork bomb.
- Xây dựng kiểm thử nhiều container đồng thời.
- Kiểm thử hồi quy root isolation.
- Xử lý lỗi dung lượng filesystem.
- Thực hiện final regression.
- Tổng hợp bảng kiểm thử T01–T16.
- Cập nhật tài liệu kiến trúc.
- Tổng hợp lỗi và hạn chế kỹ thuật.
- Viết báo cáo tiến độ tuần 4.

## 4. Đánh giá mã nguồn phát triển
(Developed Code Review)

### Ưu điểm

- Quản lý tài nguyên theo từng container.
- Sử dụng spinlock bảo vệ bộ đếm tài nguyên.
- Tích hợp memory quota với vòng đời tiến trình.
- Hỗ trợ eager và lazy allocation.
- Ghi nhận CPU runtime nhỏ hơn một timer tick.
- Duy trì hoạt động host khi container vượt quota.
- Cách ly PID và thư mục gốc.
- Có chương trình kiểm thử và bằng chứng thực nghiệm.

### Hạn chế

- CPU quota chưa phải strict quota.
- Nhiều hart có thể làm vượt ngân sách CPU.
- Chưa giải quyết hoàn toàn rủi ro sleeplock.
- Chưa hỗ trợ destroy/reuse container.
- Chưa kiểm thử đầy đủ lỗi cấp phát RAM giữa chừng.
- File descriptor kế thừa chưa cách ly tuyệt đối.
- Container-run chưa xác nhận hoàn thành.

### Đánh giá chung

Hệ thống đã đạt phần lớn mục tiêu kỹ thuật
đặt ra cho tuần 4.

Các chức năng chính đã có bằng chứng kiểm thử.

Tuy nhiên, vẫn cần tiếp tục cải tiến để nâng cao
tính ổn định và mức độ cách ly tài nguyên.

## 5. Kế hoạch tiếp theo (TODO)

- Hoàn thiện chương trình Container-run.
- Triển khai cơ chế hủy container.
- Hỗ trợ tái sử dụng container ID.
- Cải thiện CPU quota trên nhiều hart.
- Xử lý rủi ro khi throttle tiến trình giữ sleeplock.
- Tăng phạm vi kiểm thử đồng thời.
- Bổ sung fault injection cho memory quota.
- Hoàn thiện tài liệu kỹ thuật.
- Chuẩn bị demo sản phẩm.
- Chuẩn bị báo cáo tổng kết cuối kỳ.
