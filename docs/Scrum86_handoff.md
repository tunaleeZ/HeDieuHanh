# Scrum86 – Memory Quota Groundwork – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: hoàn tất

## 1. Mục tiêu và Acceptance Criteria
Ghi lại vị trí các điểm hook theo đúng luồng code hiện tại, sau đó thêm helpers reserve/release, syscall `cont_set_mem(31)` và test cấu hình quota qua `cont_info`. Task này chưa gắn accounting vào đường cấp phát/giải phóng VM.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 5.1 Survey vm.c hook points and fix the counting design | đã commit | fa0a8a7 | Bảng 7.1 đã đối chiếu vm.c, proc.c, exec.c; cập nhật rollback, owner và lazy mapping |
| 5.2 Add page reserve/release helpers and cont_set_mem syscall (31) | đã commit | fdea093 | Readback/negative/permission cases trong ctest PASS |

## 3. Bằng chứng
5.1 là thay đổi tài liệu; không chạy code test. 5.2: `make`, `make fs.img` PASS; QEMU 10.2.1, 128M, 3 hart: `ctest` PASS, 0 failures. `cont_info` đọc lại `max_pages=64`, setter từ chối số âm và lời gọi từ trong container. Raw log: `docs/test_evidence/T07_mem_config.txt`.

## 4. Giới hạn
Chỉ triển khai cấu hình `max_pages` và bộ đếm helper ở task này. Chưa gọi reserve/release từ VM allocation/free hooks; `pages_used` vì vậy chưa phản ánh resident user pages và quota chưa chặn cấp phát thực tế.

## 5. Bước tiếp theo
SCRUM-86 hoàn tất với hai commit. VM hooks được để lại cho công việc quota memory tiếp theo; không push.
