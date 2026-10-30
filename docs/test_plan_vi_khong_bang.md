# SCRUM-51 — Kế hoạch kiểm thử ban đầu: Lightweight Containers trên xv6-riscv

**Sprint:** Sprint 1 – Thiết lập môi trường và khảo sát kỹ thuật  
**Nhiệm vụ:** LC-W1-04 / SCRUM-51  
**Trạng thái:** **Chỉ lập kế hoạch, chưa chạy kiểm thử**  
**Môi trường:** xv6-riscv, build từ Makefile của nhóm và chạy bằng QEMU. Khi kiểm thử thực tế phải ghi commit Git, phiên bản QEMU, số hart và các giới hạn cấu hình.

## 1. Mục tiêu và phương pháp

Kiểm tra khả năng build/boot, cô lập PID giữa hai container, giới hạn root filesystem, và ba loại quota: **tiến trình, bộ nhớ, CPU**. Đặc biệt cần chứng minh hai container có thể nhìn thấy PID 1 riêng và bài thử tạo nhiều tiến trình có kiểm soát không làm shell **của xv6** mất phản hồi. Các phép thử được thực hiện **bên trong máy khách xv6 chạy trên QEMU**; cgroups của Ubuntu không phải cơ chế giới hạn của dự án.

Trước khi thử stress, phải có giới hạn an toàn, cách dừng bài thử và cách khởi động lại QEMU. **Không chạy fork bomb không giới hạn trên Ubuntu.**

## 2. Điều kiện tiên quyết

- Ghi rõ URL repository, branch, commit, lệnh build (`make qemu`...), phiên bản công cụ/QEMU và kết quả mong đợi.
- Cả ba thành viên đều build/boot được xv6 và chạy được ít nhất một chương trình user.
- Thống nhất bộ nhớ được đo theo trang user, trang RAM đang cư trú hay toàn bộ trang vật lý; làm rõ chu kỳ/tick CPU và ý nghĩa PID.
- Bổ sung bộ đếm/log cơ bản về cấp phát tài nguyên, hoàn tác lỗi và dọn dẹp container **trước khi kết luận quota hoạt động đúng**.
- Những tính năng container chưa được chứng minh có trong mã nguồn cung cấp; các bài dưới đây là **tiêu chí nghiệm thu dự kiến sau khi triển khai**.

## 3. Ma trận 16 ca kiểm thử

**T01 – Boot cơ bản**

**Cách thực hiện (khi đã có tính năng):** Build sạch và chạy `make qemu`

**Kết quả mong đợi:** Kernel khởi động, shell xv6 phản hồi

**Bằng chứng:** Log build và ảnh shell


**T02 – Chương trình user**

**Cách thực hiện (khi đã có tính năng):** Chạy lệnh có sẵn, sửa mã an toàn rồi build lại

**Kết quả mong đợi:** Chương trình hoạt động trước và sau build

**Bằng chứng:** Log Terminal


**T03 – PID độc lập**

**Cách thực hiện (khi đã có tính năng):** Khởi chạy 2 container, xem PID trong từng container

**Kết quả mong đợi:** Tiến trình đầu tiên được chỉ định ở mỗi container nhìn thấy PID 1; PID phía host khác

**Bằng chứng:** Log hai container


**T04 – Giới hạn tạo tiến trình**

**Cách thực hiện (khi đã có tính năng):** Đặt quota 4, tạo đến và vượt quota

**Kết quả mong đợi:** Trong quota thành công, vượt quota bị từ chối, container khác không bị ảnh hưởng

**Bằng chứng:** Bộ đếm và lỗi trả về


**T05 – Hoàn trả quota tiến trình**

**Cách thực hiện (khi đã có tính năng):** Cho con thoát/được reap rồi tạo con mới

**Kết quả mong đợi:** Quota được hoàn trả đúng một lần theo chính sách; không rò rỉ

**Bằng chứng:** Số đếm trước/sau


**T06 – Chặn fork bomb**

**Cách thực hiện (khi đã có tính năng):** Chạy bài tạo con đệ quy **có giới hạn** trong container

**Kết quả mong đợi:** Dừng tại quota; shell xv6 phía ngoài container vẫn chạy lệnh được

**Bằng chứng:** Số tiến trình và log shell


**T07 – RAM dưới quota**

**Cách thực hiện (khi đã có tính năng):** Cấu hình tối đa 32 trang dữ liệu user; cấp ít hơn mức này

**Kết quả mong đợi:** Thành công; bộ đếm trang phù hợp chính sách

**Bằng chứng:** Log bộ đếm trang


**T08 – RAM vượt quota**

**Cách thực hiện (khi đã có tính năng):** Cấp phát tăng dần đến vượt giới hạn

**Kết quả mong đợi:** Yêu cầu vượt quota thất bại an toàn; kernel không panic, container khác vẫn hoạt động

**Bằng chứng:** Log lỗi và shell


**T09 – Thu hồi RAM và fork**

**Cách thực hiện (khi đã có tính năng):** Cấp phát, giải phóng/thu nhỏ hoặc exit; thử fork nhiều lần

**Kết quả mong đợi:** Trang được hoàn trả; không trừ/cộng hai lần; lỗi fork có hoàn tác

**Bằng chứng:** Log và bất biến bộ đếm


**T10 – CPU quota**

**Cách thực hiện (khi đã có tính năng):** Chạy 2 container dùng CPU liên tục: một quota thấp, một quota cao/không giới hạn

**Kết quả mong đợi:** Tick CPU đo được theo chu kỳ phù hợp quota trong sai số đã quy định

**Bằng chứng:** Tick và chu kỳ


**T11 – Làm mới quota CPU**

**Cách thực hiện (khi đã có tính năng):** Dùng hết quota, chờ sang chu kỳ tiếp theo

**Kết quả mong đợi:** Tiến trình được chạy lại, không bị đói CPU vĩnh viễn

**Bằng chứng:** Dòng thời gian tick


**T12 – CPU nhiều hart**

**Cách thực hiện (khi đã có tính năng):** Chạy QEMU với hơn 1 hart nếu hỗ trợ

**Kết quả mong đợi:** Tổng quota được thực thi an toàn; không race/deadlock

**Bằng chứng:** Số hart và các lượt thử


**T13 – Chống thoát root**

**Cách thực hiện (khi đã có tính năng):** Truy cập file trong root; thử `..`, đường dẫn tuyệt đối, trường hợp file descriptor kế thừa

**Kết quả mong đợi:** Truy cập hợp lệ thành công; chặn đường thoát root

**Bằng chứng:** Log lệnh truy cập


**T14 – Nhiều container đồng thời**

**Cách thực hiện (khi đã có tính năng):** Hai container có quota/root khác nhau cùng chạy

**Kết quả mong đợi:** Một container hết tài nguyên không làm hỏng container kia

**Bằng chứng:** Log trạng thái song song


**T15 – Dọn dẹp và tái sử dụng**

**Cách thực hiện (khi đã có tính năng):** Tạo/hủy container nhiều lần

**Kết quả mong đợi:** Không còn thành viên/quota cũ, không rò rỉ ô quản lý

**Bằng chứng:** Bộ đếm và log nhiều lượt


**T16 – Hồi quy**

**Cách thực hiện (khi đã có tính năng):** Chạy lại các lệnh user gốc của xv6 sau khi sửa kernel

**Kết quả mong đợi:** Vẫn boot và chạy chức năng cũ bình thường

**Bằng chứng:** Kết quả lệnh



**Các giá trị 4 tiến trình và 32 trang chỉ là cấu hình gợi ý, không phải kết quả đã kiểm thử.**

## 4. Ghi nhận kết quả PASS/FAIL

Với mỗi lượt thử, điền mẫu sau:

```text
Mã test: T__
Git commit / branch:
Phiên bản QEMU / số hart:
Container ID / quota cấu hình:
Lệnh và tải thử:
Kết quả mong đợi:
Kết quả thực tế:
Số trang RAM / số tiến trình / tick CPU (nếu liên quan):
Trạng thái: PASS / FAIL / BLOCKED / NOT RUN
File minh chứng hoặc link Jira:
Người thực hiện / ngày / lỗi ghi nhận:
```

### Nguyên tắc đánh giá

- **Tiến trình:** thống nhất khi nào một tiến trình được tính vào quota; thử cả fork thất bại và `ZOMBIE`/`wait`.
- **Bộ nhớ:** tính đúng các sự kiện cấp/thu hồi trang đã quy định, **không dùng riêng `struct proc.sz`** để đại diện RAM vật lý; kiểm tra thu nhỏ, exit, fork, exec và cấp phát lỗi.
- **CPU:** đo tick chạy thực tế qua nhiều chu kỳ với sai số phù hợp độ phân giải timer; ghi rõ số hart. Không đánh đồng thời gian ảo QEMU với thời gian thực của Ubuntu.
- **Cô lập:** thử tác động chéo giữa container; bằng chứng shell phản hồi phải là shell **bên trong xv6**, không chỉ Terminal Ubuntu.

## 5. Bằng chứng lưu trong repository/Jira

Cấu trúc gợi ý:

```text
docs/
  architecture_draft.md
  test_plan.md
  evidence/
    README.md
    T01_boot.txt
    T03_pid_views.txt
    T06_process_cap.txt
    T08_memory_limit.txt
    T10_cpu_quota.txt
```

Chỉ tạo file chứa kết quả khi đã thực sự chạy test. Không đánh dấu PASS chỉ vì ca kiểm thử được viết trong tài liệu.

## 6. Thứ tự triển khai/kiểm thử đề xuất cho tuần 2

1. Thống nhất cách gắn tiến trình vào container và quyền sở hữu đối tượng.
2. Thêm bộ đếm nền cho tiến trình, RAM và CPU; xác minh cách cập nhật/đặt lại.
3. Thử giới hạn tiến trình, ưu tiên T04–T06 trước bất kỳ bài stress lớn nào.
4. Triển khai quota trang user được định nghĩa rõ; kiểm tra T07–T09.
5. Thử quota dựa trên scheduler/tick; kiểm tra T10–T12.
6. Tích hợp công việc PID/filesystem, kiểm tra T03/T13/T14.
7. Chạy lại boot, user-space, dọn dẹp và hồi quy: T01/T02/T15/T16.

## 7. Checklist Sprint 1

- [x] Đã soạn ma trận test ban đầu và loại bằng chứng dự kiến.
- [ ] Thống nhất người thực hiện và lệnh chạy từng ca với các thành viên.
- [ ] Thống nhất, triển khai bộ đếm/log đo lường (tuần 2 hoặc sau).
- [ ] Thu thập log thực tế và kết quả PASS/FAIL sau khi triển khai.
- [ ] Hoàn tất đường dẫn Jira, blockers và backlog tuần 2.

**Cơ sở biên soạn:** tài liệu kế hoạch tuần 1 của nhóm (mục 2, 7, 8, 9, 10) và trao đổi SCRUM-51. Tài liệu cố ý phân biệt **kế hoạch sẽ kiểm thử** với **kết quả đã chạy**.
