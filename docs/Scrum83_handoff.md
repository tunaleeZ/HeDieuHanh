# Scrum83 – Container-Level Root Directory – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang làm

## 1. Mục tiêu và Acceptance Criteria
Chuyển root thành thuộc tính container theo D1: host cấu hình root một lần bằng cont_setroot; tiến trình nhận root/cwd của container khi cont_enter. Bổ sung regression tests và fixture tái lập được. Không làm permission host-only của SCRUM-84 trong task này.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái (chưa làm / đã đề xuất / đã duyệt / đã commit) | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 2.1 Add root to struct container and cont_setroot syscall (29) | đã commit | c62d214 | Setter nhận inode đã được resolve/kiểm tra |
| 2.2 Apply container root in cont_enter | đã duyệt | — | Snapshot root có lock; idup ngoài lock; iput cũ trong transaction |
| 2.3 Extend roottest with uncovered cases | chưa làm | — | Thêm FS operations, final `..`, fork, pre-open fd, container flow |
| 2.4 Script the /c1 fixture and save evidence | chưa làm | — | Tạo user fixture tool và lưu T13 log |

## 3. File đã đổi (theo sub-task)
Sub-task 2.1: kernel/proc.c, kernel/defs.h, kernel/sysfile.c, kernel/sysproc.c. Sub-task 2.2: kernel/proc.c. Dự kiến tiếp theo: user/roottest.c, user/setupc1.c, Makefile, docs/test_evidence/T13_root_escape.txt.

## 4. Quyết định kỹ thuật và lý do
`struct container.root` và syscall wrapper 29 đã có từ SCRUM-89. `kcontainer_setroot()` sẽ nhận inode đã được resolve/kiểm tra trong sysfile.c; khi thành công, container sở hữu reference trả về từ namei(). Root được set một lần và không có destroy container. `idup()` chỉ dùng itable spinlock; khi lấy root từ container, sao chép pointer dưới container lock rồi idup ngoài lock vì root bất biến sau khi set. `iput()` luôn ngoài container lock và trong transaction.

## 5. Bằng chứng kiểm thử
Sub-task 2.1: PASS `make clean`, `make`, `make fs.img`. Sub-task 2.2: PASS `make`. Runtime T13 CHƯA CHẠY; sẽ lưu sau khi fixture và roottest được hoàn tất.

## 6. Phát hiện / vấn đề mở / rủi ro
SCRUM-84 sẽ bổ sung host-only permission và p->container lock fix. Task này không thêm permission check. Root inode của container giữ một reference suốt vòng đời container; hiện chưa có destroy path.

## 7. Bước tiếp theo
Hoàn tất build sub-task 2.2 rồi commit `feat: Apply container root in cont_enter`. Tiếp tục test/fixture 2.3–2.4 và lưu bằng chứng T13.

## 8. Lưu ý khi tiếp tục trong chat mới
Khung 8.2 đã có (`cpu_period_ticks` trong kernel/proc.h). Làm sub-task theo thứ tự, mỗi sub-task một commit. Chỉ ghi PASS dựa trên output thực tế; mọi lệnh QEMU cần timeout.
