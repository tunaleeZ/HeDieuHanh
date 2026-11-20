# SCRUM-96: Container Run Tool Prototype — Handoff

## 1. Mục tiêu & Acceptance Criteria

### Mục tiêu
Xây dựng công cụ tiện ích user-space `container-run` để tạo, cấu hình tài nguyên và khởi chạy chương trình bên trong lightweight container từ xv6 shell.

### Acceptance Criteria
- [ ] **AC-1:** Có thể tạo và chạy chương trình trong container qua công cụ user-space.
- [ ] **AC-2:** Container root được thiết lập an toàn trước khi tiến trình đi vào.
- [ ] **AC-3:** Cấu hình process, CPU và memory quota được truyền đúng qua API.
- [ ] **AC-4:** Trường hợp lỗi được xử lý an toàn, không làm hỏng shell host.
- [ ] **AC-5:** Có đầy đủ demo, mã nguồn, log và tài liệu hướng dẫn.

---

## 2. Trạng thái Sub-tasks

| Sub-task | Trạng thái | Commit | Ghi chú |
|---|---|---|---|
| 5.1 Design CLI | Hoàn tất | `docs: Design CLI` | Thiết kế chi tiết CLI, quy trình an toàn 0-6, xử lý edge case tại `docs/container_run_design.md`. |
| 5.2 Implement container-run | Chưa chạy | - | Xây dựng `user/container-run.c` và tích hợp vào `Makefile`. |
| 5.3 Integration demo | Chưa chạy | - | Thực hiện kịch bản demo S1–S10 trên fixture `/c1`. |
| 5.4 Documentation | Chưa chạy | - | Soạn thảo `docs/container_run_guide.md` và hoàn thiện handoff. |

---

## 3. Quyết định Kỹ thuật Chính

1. **Quy trình an toàn 7 bước (0–6):**
   - Tách biệt kiểm tra cú pháp và pre-flight check phía host trước khi gọi `cont_create`. Điều này ngăn chặn việc lãng phí 7 slot container do lỗi gõ lệnh hoặc file không tồn tại.
   - Luôn thiết lập `cont_setroot` trước khi `fork` và `cont_enter`.
   - Tiến trình cha tuyệt đối không bao giờ gọi `cont_enter` để giữ toàn vẹn quyền hạn host.
2. **Quy ước Quota CPU $0:period$:**
   - Được xác định là **hợp lệ** và biểu thị trạng thái **không giới hạn CPU (unthrottled)**, hoàn toàn khớp với logic kernel `limit = cpu_quota_ticks * CPU_TICK_RUNTIME = 0` (bỏ qua throttle).
3. **Mở rộng tính năng `-i <id>` và `-l`:**
   - Triển khai `-l` để quan sát trạng thái 8 slot container qua `cont_info`.
   - Triển khai `-i <id>` cho phép chạy thêm tiến trình vào container đã tạo sẵn, giúp tiết kiệm slot trong các kịch bản demo phức tạp.

---

## 4. Bằng chứng & Log Kiểm thử
*(Sẽ được cập nhật chi tiết sau khi thực hiện Sub-task 5.2 và 5.3)*

---

## 5. Rủi ro & Phát hiện Kỹ thuật
- Bảng container của xv6 không hỗ trợ `cont_destroy`, do đó số lần tạo mới container tối đa là 7 lần mỗi phiên boot QEMU.
- Tên chương trình `container-run` có độ dài 13 ký tự, nằm trong giới hạn `DIRSIZ=14` của xv6 filesystem.

---

## 6. Bước tiếp theo
- Hoàn tất commit `docs: Design CLI`.
- Chuyển sang Sub-task 5.2: Trình duyệt code `user/container-run.c` và thay đổi `Makefile`.
