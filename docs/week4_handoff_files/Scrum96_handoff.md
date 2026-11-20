# SCRUM-96: Container Run Tool Prototype — Handoff

## 1. Mục tiêu & Acceptance Criteria

### Mục tiêu
Xây dựng công cụ tiện ích user-space `container-run` để tạo, cấu hình tài nguyên và khởi chạy chương trình bên trong lightweight container từ xv6 shell.

### Acceptance Criteria
- [x] **AC-1:** Có thể tạo và chạy chương trình trong container qua công cụ user-space (Bằng chứng: S1, S4, S5, S6, S8).
- [x] **AC-2:** Container root được thiết lập an toàn trước khi tiến trình đi vào (Bằng chứng: S1, S8, `roottest`).
- [x] **AC-3:** Cấu hình process, CPU và memory quota được truyền đúng qua API (Bằng chứng: S4, S5, S6).
- [x] **AC-4:** Trường hợp lỗi được xử lý an toàn, không làm hỏng shell host (Bằng chứng: S2, S3, S7, S10).
- [ ] **AC-5:** Có đầy đủ demo, mã nguồn, log và tài liệu hướng dẫn (Đang tiến hành, hoàn tất ở 5.4).

---

## 2. Trạng thái Sub-tasks

| Sub-task | Trạng thái | Commit | Ghi chú |
|---|---|---|---|
| 5.1 Design CLI | Hoàn tất | `cda2e28` | Thiết kế chi tiết CLI, quy trình an toàn 0-6, xử lý edge case tại `docs/container_run_design.md`. |
| 5.2 Implement container-run | Hoàn tất | `1f2d682` | Cài đặt `user/container-run.c` đầy đủ tính năng; build sạch và kiểm thử cơ bản PASS. |
| 5.3 Integration demo | Hoàn tất | `test: Integration demo` | Thực nghiệm đầy đủ 10 kịch bản S1–S10 và regression tests, log lưu tại `docs/test_evidence/container_run_demo.txt`. |
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

Toàn văn log thực nghiệm được lưu tại: [docs/test_evidence/container_run_demo.txt](file:///home/tuan/xv6_lab/xv6-riscv/docs/test_evidence/container_run_demo.txt).

### 4.1. Bảng kết quả 10 kịch bản Integration Demo (S1 – S10)

| Mã | Kịch bản | Lệnh thực hiện | Kết quả thực tế | Trạng thái |
|---|---|---|---|---|
| **S1** | Khởi chạy cơ bản | `container-run /c1 /echo hello` | In `hello`, mã thoát 0 | **PASS** |
| **S2** | Root không hợp lệ | `container-run /no_such_dir /echo` | Báo lỗi pre-flight, `container-run -l` xác nhận không mất slot | **PASS** |
| **S3** | Lệnh không có trong root | `container-run /c1 /no_such_cmd` | Báo lỗi pre-flight binary, `container-run -l` xác nhận không mất slot | **PASS** |
| **S4** | Process Quota Enforcement | `container-run -p 1 /c1 /bench_proc` | 100/100 lần fork thất bại bên trong container vì process chính đã chiếm slot | **PASS** |
| **S5** | CPU Quota Rate Limiting | `container-run -c 5:20 /c1 /bench_cpu` | Host: 6019 it/tick $\rightarrow$ Container: 1555 it/tick (~25.8%, khớp quota 5/20) | **PASS** |
| **S6** | Memory Quota Enforcement | `container-run -v -m 8 /c1 /bench_mem` | Verbose in `max_pages=8`, `exec` bị kernel từ chối vì binary vượt quá 8 trang | **PASS** |
| **S7** | Tham số không hợp lệ | `container-run -c 30:20 /c1 /echo`<br/>`container-run -p -1 /c1 /echo`<br/>`container-run` | Báo lỗi tham số / usage, thoát mã 1, shell host chạy lệnh `ls` bình thường | **PASS** |
| **S8** | Filesystem Isolation | `container-run /c1 /cat /outside.txt` | Bị từ chối (`cat: cannot open /outside.txt`), host đọc được "secret" | **PASS** |
| **S9** | Liệt kê & Quản lý | `container-run -l` | In chi tiết bảng 8 slot container (active, proc, CPU quota, memory) | **PASS** |
| **S10**| Cạn slot container | Tạo liên tục 7 container, gọi container thứ 8 | Container 8 báo `no free slots available`, shell host không bị treo | **PASS** |

### 4.2. Bảng kết quả kiểm thử hồi quy (Regression Verification)

| Bộ kiểm thử | Số ca lỗi | Trạng thái |
|---|---|---|
| `roottest` | 0 failures | **PASS** |
| `chroottest /c1` | 0 failures | **PASS** |
| `ptest_neg` | 0 failures | **PASS** |
| `ctest` | 0 failures | **PASS** |
| `forktest` | 0 failures | **PASS** |

---

## 5. Rủi ro & Phát hiện Kỹ thuật
- Khi container process (PID 1 trong namespace) kết thúc, namespace bị đóng theo thiết kế SCRUM-93. Cờ `-i <id>` sẽ bị từ chối vào container nếu PID 1 của container đó đã exit.
- `exec` của xv6 yêu cầu tối thiểu số trang tương ứng kích thước ELF + stack + guard page. Với `-m 8`, các binary lớn hơn 8 trang (như `bench_mem` ~ 36 KB) sẽ bị chặn ngay tại bước `exec`.

---

## 6. Bước tiếp theo
- Thực hiện commit `test: Integration demo`.
- Chuyển sang Sub-task 5.4: Soạn thảo tài liệu hướng dẫn [docs/container_run_guide.md](file:///home/tuan/xv6_lab/xv6-riscv/docs/container_run_guide.md) và hoàn tất hồ sơ bàn giao.
