# SCRUM-51 — Bản thảo kiến trúc Lightweight Containers trên xv6-riscv

Sprint: Sprint 1 – Thiết lập môi trường và khảo sát kỹ thuật 
Nhiệm vụ: LC-W1-04 / SCRUM-51 – Nghiên cứu quản lý tài nguyên và lập kế hoạch kiến trúc 
Trạng thái: Bản thảo để nhóm xem xét; chưa triển khai trong kernel
Phạm vi: xv6-riscv, không sử dụng trực tiếp Linux cgroups.

## 1. Mục tiêu và giới hạn

Đề xuất kiến trúc ban đầu của lightweight container **bên trong xv6-riscv**, kết hợp: 
(1) cô lập tiến trình và cách nhìn PID, 
(2) giới hạn thư mục gốc của container 
(3) giới hạn số tiến trình, bộ nhớ, CPU theo từng container. 
Sprint 1 tập trung khảo sát mã nguồn và lập kế hoạch; **không mặc định** xv6 đã có `chroot`, PID namespace hoặc cgroups như Linux.

### Bằng chứng và nội dung chưa xác minh

- **Đã quan sát qua ảnh Terminal:** `kernel/kalloc.c` có `kalloc()` (dòng 69), `kfree()` (dòng 47); `kernel/proc.c` có `allocproc()` (dòng 110), `scheduler()` (dòng 429); `kernel/proc.h` chứa `struct proc` với `pid`, `state`, `parent`, `sz`, `pagetable`, `context`.
- **Đã quan sát qua kết quả tìm kiếm hàm:** `kfork()` ở dòng 259, `kexit()` ở dòng 326.
- **Chưa xác minh trực tiếp:** toàn bộ luồng cấp phát trong `kernel/vm.c`, cách tính timer tick, chi tiết `kfork()`/`freeproc()`, cơ chế phân giải đường dẫn và những thay đổi riêng của nhánh Git nhóm. Những đề xuất bên dưới phải được đối chiếu lại trước khi lập trình.

## 2. Các vị trí mã nguồn cần nghiên cứu

**Trang vật lý**
File / hàm:kernel/kalloc.c: kalloc, kfree
Cơ chế hiện tại / câu hỏi khảo sát:Danh sách trang trống kmem.freelist được bảo vệ bởi kmem.lock; mỗi trang 4 KiB trong mã nguồn được chụp.
Hướng mở rộng đề xuất:Không cộng trực tiếp mọi lần gọi kalloc(): phải xác định chủ sở hữu và xử lý hoàn tác khi lỗi.
**Bộ nhớ tiến trình:**
File / hàm:kernel/vm.c: uvmalloc, uvmdealloc, uvmfree, uvmcopy
Cơ chế hiện tại / câu hỏi khảo sát:Cần kiểm tra trong mã nguồn: cấp phát, giải phóng và sao chép bộ nhớ user.
Hướng mở rộng đề xuất:Điểm có thể ghi nhận/cập nhật số trang cho container.
**Cấu trúc tiến trình:**
File / hàm:kernel/proc.h: struct proc
Cơ chế hiện tại / câu hỏi khảo sát:Chứa PID, trạng thái vòng đời, không gian địa chỉ và context CPU.
Hướng mở rộng đề xuất:Bổ sung ID hoặc con trỏ trỏ tới container.
**Bộ nhớ tiến trình:**
File / hàm:
Cơ chế hiện tại / câu hỏi khảo sát:
Hướng mở rộng đề xuất:
**Tạo tiến trình:**
File / hàm:kernel/proc.c: allocproc, kfork, freeproc
Cơ chế hiện tại / câu hỏi khảo sát:allocproc tìm ô UNUSED, cấp PID, tạo trapframe và page table.
Hướng mở rộng đề xuất:Kiểm tra quota tiến trình, kế thừa thành viên, hoàn tác khi thất bại.
**Lập lịch CPU:**
File / hàm:kernel/proc.c: scheduler
Cơ chế hiện tại / câu hỏi khảo sát:Duyệt tiến trình RUNNABLE, dùng swtch chuyển ngữ cảnh.
Hướng mở rộng đề xuất:Không chọn tiến trình thuộc container đã dùng hết quota, cho chạy lại ở kỳ sau.
**Timer/ngắt:**
File / hàm:kernel/trap.c: luồng timer
Cơ chế hiện tại / câu hỏi khảo sát:Cần kiểm tra: timer tick, yield và nhiều hart.
Hướng mở rộng đề xuất:Đếm mức sử dụng CPU và đặt lại quota định kỳ.
**Gốc hệ thống file:**
File / hàm:kernel/fs.c, kernel/sysfile.c, kernel/exec.c
Cơ chế hiện tại / câu hỏi khảo sát:Thành viên phụ trách filesystem nghiên cứu; chưa có kết luận cuối.
Hướng mở rộng đề xuất:Ngăn truy cập đường dẫn vượt ngoài gốc container.
**Công cụ người dùng:**
File / hàm:user/ và các file syscall
Cơ chế hiện tại / câu hỏi khảo sát:Đề xuất thiết kế.
Hướng mở rộng đề xuất:Lệnh container để chạy/xem trạng thái và syscall điều khiển tối thiểu.

## Kiến trúc được đề xuất

```text
                    Không gian user của xv6
            +---------------------------------+
            | Công cụ container-run / inspect |
            +----------------+----------------+
                             | Cấu hình container
                        System call
                             v
                  xv6 kernel (dự kiến)
  +-----------------------------------------------------------+
  | Quản lý container                                         |
  | ID, vòng đời, thành viên, quota, mức sử dụng, khóa         |
  +----------------+-----------------+------------------------+
                   |                 |                |
          Tiến trình / PID        Bộ nhớ             CPU
          proc.c / proc.h       vm.c/kalloc.c      proc.c/trap.c
          fork/exit/wait        cấp/thu hồi trang  tick/scheduler
                   |                 |                |
  +----------------+-----------------+------------------------+
  | Ranh giới hệ thống file (phối hợp thành viên filesystem)   |
  +-----------------------------------------------------------+
```

**Đây là sơ đồ logic.** Nhóm cần thống nhất giao diện giữa các thành phần và chữ ký system call.

## 4. Mô hình dữ liệu dự kiến

Ví dụ minh họa bằng C, 

```c
struct container {
    int id;
    int active;
    int process_count;
    int max_processes;
    unsigned long used_pages;
    unsigned long max_pages;
    unsigned long used_cpu_ticks;
    unsigned long cpu_quota_ticks;
    unsigned long cpu_period_ticks;
    // Can thiet ke them: lock, root inode, bang PID, trang thai vong doi
};

// Du kien bo sung vao struct proc:
// struct container *container;
```

Giai đoạn đầu có thể sử dụng mảng container kích thước cố định cho phù hợp với xv6. Cần quy định ai sở hữu đối tượng container, thứ tự lấy khóa và điều kiện giải phóng container. Nếu QEMU dùng nhiều hart, các hart có thể lập lịch đồng thời nên không được cập nhật quota bằng biến đếm thiếu đồng bộ.

## 5. Chiến lược giới hạn tài nguyên

**Số tiến trình:**
Cách theo dõi/kiểm tra đề xuất: Khi tạo tiến trình con, kế thừa container của cha; giữ chỗ nguyên tử trước khi cho con chạy; hoàn trả khi fork lỗi hoặc khi tiến trình kết thúc ở giai đoạn vòng đời đã thống nhất.
Khi vượt giới hạn: Từ chối tạo tiến trình mới nếu process_count >= max_processes; không tự động giết tiến trình đang chạy.
Vấn đề cần kiểm chứng: Có tính trạng thái ZOMBIE vào quota không? Trừ bộ đếm lúc exit hay khi thu hồi bằng wait? Phải tránh trừ hai lần, xử lý tiến trình mồ côi và quy định thứ tự khóa.

**Bộ nhớ:**
Cách theo dõi/kiểm tra đề xuất: Trước tiên xác định phạm vi kế toán. Giai đoạn đầu có thể đếm các trang dữ liệu user thuộc container và ghi nhận chủ sở hữu; chỉ tăng bộ đếm khi cấp phát thành công, giảm khi thu hồi hoặc hoàn tác.
Khi vượt giới hạn: Yêu cầu cấp phát vượt quota thất bại an toàn, nhưng kernel vẫn tiếp tục hoạt động.
Vấn đề cần kiểm chứng: kalloc() còn phục vụ kernel; cần xử lý fork, page table, trapframe, thay thế bộ nhớ khi exec, thu nhỏ bằng sbrk và hoàn tác khi lỗi. p->sz không đồng nghĩa với RAM vật lý thực tế.

**CPU:**
Cách theo dõi/kiểm tra đề xuất: Đếm tick CPU khi tiến trình của container đang chạy; đặt lại số tick theo chu kỳ và cộng mức sử dụng trên nhiều hart.
Khi vượt giới hạn: Scheduler tạm bỏ qua các tiến trình thuộc container đã hết quota; cho phép chạy lại khi chu kỳ mới bắt đầu.
Vấn đề cần kiểm chứng: Độ phân giải timer, khả năng vượt quota một lượng nhỏ, thời gian ngủ/nhàn rỗi, tính công bằng, tránh starvation và đồng bộ khi có nhiều CPU.

**Thông số minh họa để kiểm thử (không phải yêu cầu cố định):** Tối đa 4 tiến trình, 32 trang dữ liệu user (128 KiB), 5 tick chạy trong mỗi chu kỳ 20 tick. Cần hiệu chỉnh sau khi khảo sát timer và các chương trình thử xv6.
## 6. Vòng đời container và xử lý lỗi dự kiến

1. Công cụ user gửi cấu hình thư mục gốc và giới hạn tài nguyên; kernel kiểm tra hợp lệ.
2. Tạo và khởi tạo đối tượng quản lý container.
3. Gán tiến trình đầu tiên vào container; phối hợp với phần PID và filesystem của thành viên khác.
4. Khi `fork`, tiến trình con kế thừa container và được kiểm tra giới hạn; các trang nhớ được ghi nhận theo chính sách đã chọn.
5. Scheduler theo dõi CPU và áp dụng giới hạn theo chu kỳ.
6. Khi `exit`, `wait` và giải phóng tài nguyên, cập nhật đúng bộ đếm; chỉ hủy container khi an toàn.
7. Mọi nhánh lỗi phải hoàn tác tài nguyên đã giữ chỗ, tránh rò rỉ hoặc deadlock.

## 7. Ánh xạ yêu cầu sang thiết kế

**ai container đều nhìn thấy PID 1 riêng**
Cơ chế dự kiến: Ánh xạ PID ảo theo container, kế thừa thành viên.
Người phụ trách/phụ thuộc: Thành viên process.
Bằng chứng cần có: Kiểm thử PID ở hai container.

**Root filesystem riêng**
Cơ chế dự kiến: Gốc inode, kiểm soát phân giải đường dẫn.
Người phụ trách/phụ thuộc: Thành viên filesystem.
Bằng chứng cần có: Kiểm thử cô lập đường dẫn.

**Giới hạn tiến trình**
Cơ chế dự kiến: Giữ chỗ và kiểm tra nguyên tử khi fork.
Người phụ trách/phụ thuộc: SCRUM-51 + thành viên process.
Bằng chứng cần có: Kiểm thử quota tiến trình.

**Giới hạn RAM**
Cơ chế dự kiến: Theo dõi quyền sở hữu và cấp/thu hồi trang.
Người phụ trách/phụ thuộc: SCRUM-51.
Bằng chứng cần có: Thử cấp phát vượt giới hạn.

**Giới hạn CPU**
Cơ chế dự kiến: Đếm tick và scheduler có kiểm tra quota.
Người phụ trách/phụ thuộc: SCRUM-51.
Bằng chứng cần có: Kiểm thử quota và tính công bằng.

**Fork bomb không làm treo shell chủ**
Cơ chế dự kiến: Quota tiến trình và bài stress được giới hạn.
Người phụ trách/phụ thuộc: Cả nhóm.
Bằng chứng cần có: Shell bên trong xv6 vẫn phản hồi.

**Công cụ chạy container**
Cơ chế dự kiến: Lệnh user và syscall được chọn.
Người phụ trách/phụ thuộc: Cả nhóm.
Bằng chứng cần có: Demo chạy/xem trạng thái/dọn dẹp.

**UTS namespace, private tmpfs**
Cơ chế dự kiến: Tính năng mở rộng nếu còn thời gian.
Người phụ trách/phụ thuộc: Cả nhóm.
Bằng chứng cần có: Chỉ làm sau phần cơ bản.

## 8. Các quyết định cần thống nhất trong nhóm

- Dùng `container_id` hay `struct container *` trong `struct proc`; ai chịu trách nhiệm vòng đời đối tượng.
- Giới hạn RAM tính riêng trang user hay toàn bộ trang vật lý (kể cả kernel); quy định phạm vi rõ ràng.
- Ý nghĩa phần trăm CPU khi có nhiều hart; chu kỳ quota và nguồn timer tick.
- Tiến trình `ZOMBIE` có được tính vào quota? Thời điểm trả lại quota.
- Ánh xạ PID ảo, PID 1, `kill`, `wait` và cách chuyển đổi PID.
- Ngăn vượt gốc filesystem qua `..`, đường dẫn tuyệt đối và file descriptor kế thừa.
- API user/kernel và mã lỗi (ví dụ `-1` kèm nguyên nhân được mô tả).

## 9. Backlog đề xuất cho tuần 2

1. **Nhóm process:** thử nghiệm gắn tiến trình với container và giới hạn số tiến trình.
2. **Nhóm filesystem:** thử nghiệm thư mục gốc riêng và kiểm soát đường dẫn.
3. **Người phụ trách SCRUM-51:** bổ sung công cụ đo số trang nhớ/tick CPU, xác lập kết quả nền khi chưa có quota.
4. **Cả nhóm:** thống nhất `struct container`, quy tắc đồng bộ và xử lý lỗi.
5. **Cả nhóm:** chạy lại build/boot và chương trình user mẫu sau mỗi lần gộp code.

## 10. Checklist hoàn thành Sprint 1

- [ ] Cả ba thành viên build, boot và chạy chương trình user trong xv6; có ảnh/log làm bằng chứng.
- [x] Đã xác định điểm mã nguồn cấp phát trang, tạo tiến trình và scheduler từ ảnh/log được cung cấp.
- [ ] Xác minh thêm luồng bộ nhớ, timer, thu hồi tiến trình trên đúng nhánh Git nhóm.
- [ ] Tổng hợp kết quả của thành viên process và filesystem.
- [x] Đã có bản thảo giới hạn tài nguyên, sơ đồ kiến trúc và ánh xạ yêu cầu.
- [x] Đã có ma trận kiểm thử ban đầu trong `docs/week1/test_plan_vi_khong_bang.md`.
- [ ] Nhóm rà soát Jira backlog, phân công và bằng chứng.

**Cơ sở biên soạn:** tài liệu nhóm “Nhiệm vụ Tuần 1 – Lightweight Containers xv6-riscv” (các mục 1, 2, 7, 8, 9, 10) và ảnh/log mã nguồn đã được cung cấp. Mọi chi tiết triển khai chưa hiện trong bằng chứng đều phải được kiểm chứng trên repository của nhóm.
