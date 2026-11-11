# Scrum89 – Tag baseline and push shared skeleton – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: xong chờ review

## 1. Mục tiêu và Acceptance Criteria
Hoàn thành khung chung Sub-task 8.2 cho các task 83, 85, 86, 87: bổ sung trường container và container_info đồng bộ kernel/user, khởi tạo trường mới, thêm syscall stub 29–31 trả -1. Không thêm logic nghiệp vụ. Tag week2-stable chỉ tạo cục bộ trước khi sửa code nếu được duyệt. Build và các kiểm thử yêu cầu phải đạt.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái (chưa làm / đã đề xuất / đã duyệt / đã commit) | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 8.1 | chưa làm | — | Ngoài phạm vi |
| 8.2 | đã duyệt | — | Đã triển khai và kiểm chứng; chờ commit |
| 8.3 | chưa làm | — | Ngoài phạm vi |

## 3. File đã đổi (theo sub-task)
kernel/proc.h, kernel/proc.c, kernel/syscall.h, kernel/syscall.c, kernel/sysproc.c, user/user.h, user/usys.pl, docs/week3_handoff_files/Scrum89_handoff.md, docs/test_evidence/scrum89_subtask82_qemu.log.

## 4. Quyết định kỹ thuật và lý do
container_info được khai báo riêng ở kernel/proc.h và user/user.h; hai bản giữ cùng thứ tự trường để đồng bộ layout. Các syscall mới chỉ là stub trả -1, chưa xử lý nghiệp vụ.

## 5. Bằng chứng kiểm thử
PASS: kiểm tra trùng số syscall (không có giá trị trùng); make clean; make; make fs.img; ctest; roottest với fixture thủ công; test_sysinfo. Chi tiết output trong docs/test_evidence/scrum89_subtask82_qemu.log. QEMU 10.2.1, 3 hart, 128M RAM.

## 6. Phát hiện / vấn đề mở / rủi ro
Tag week2-stable được tạo cục bộ tại baseline b48ec18 trước khi sửa code. Chưa push tag.

## 7. Bước tiếp theo
Đối chiếu git status/diff và commit đúng một commit: feat: Tag baseline and push shared skeleton. Không push tag.

## 8. Lưu ý khi tiếp tục trong chat mới
Chỉ làm Sub-task 8.2 của SCRUM-89. Chưa làm 8.1 hoặc 8.3. Không push tag.
