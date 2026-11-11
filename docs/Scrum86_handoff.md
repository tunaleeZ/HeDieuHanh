# Scrum86 – Memory Quota Groundwork – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang thực hiện

## 1. Mục tiêu và Acceptance Criteria
Ghi lại vị trí các điểm hook theo đúng luồng code hiện tại, sau đó thêm helpers reserve/release, syscall `cont_set_mem(31)` và test cấu hình quota qua `cont_info`. Task này chưa gắn accounting vào đường cấp phát/giải phóng VM.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 5.1 Survey vm.c hook points and fix the counting design | đã commit | sẽ ghi sau | Bảng 7.1 đã đối chiếu vm.c, proc.c, exec.c; cập nhật rollback, owner và lazy mapping |
| 5.2 Add page reserve/release helpers and cont_set_mem syscall (31) | chưa làm | — | |

## 3. Bằng chứng
5.1 là thay đổi tài liệu; không chạy code test. Bảng hook giữ nguyên các mục đo lường hiện có trong `memory_accounting.md` và ghi rõ call path/owner từ code đang có.

## 4. Giới hạn
Chỉ triển khai cấu hình `max_pages` và bộ đếm helper ở task này. Chưa gọi reserve/release từ VM allocation/free hooks; `pages_used` vì vậy chưa phản ánh resident user pages.

## 5. Bước tiếp theo
Hoàn tất khảo sát tài liệu 5.1 rồi triển khai 5.2. Không push.
