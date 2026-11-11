# Scrum82 – Resolve Open Design Questions for Week 3 – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang làm

## 1. Mục tiêu và Acceptance Criteria
Chốt bảy câu hỏi trong mục 7 của `docs/architecture_review_scrum64.md` theo các quyết định D1–D6; hoàn tất API và tham số trong `docs/cpu_quota_design.md`; bổ sung thiết kế quota vào `docs/memory_accounting.md` sau khi đối chiếu kernel/vm.c, proc.c và exec.c. Chỉ sửa Markdown. Người dùng sẽ tự xác nhận quyết định trên Jira.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái (chưa làm / đã đề xuất / đã duyệt / đã commit) | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 1.1 Answer the 7 open questions in architecture_review_scrum64.md | đã duyệt | — | Nội dung hoàn tất, chờ commit; có mục 7, bảng syscall và quy ước lock |
| 1.2 Finalize cpu_quota_design.md | chưa làm | — | Giữ nguyên kế hoạch CPU-01..CPU-09 |
| 1.3 Add quota design section to memory_accounting.md | chưa làm | — | Chỉ ghi điểm hook đã xác minh từ mã nguồn |

## 3. File đã đổi (theo sub-task)
Sub-task 1.1: `docs/architecture_review_scrum64.md`, `docs/Scrum82_handoff.md`.

## 4. Quyết định kỹ thuật và lý do
Áp dụng quyết định D1–D6 đã chốt trong yêu cầu SCRUM-82. Sub-task 1.1 ghi cả Quyet dinh, Ly do, He qua cho từng câu hỏi, cùng các số syscall 28–31 và thứ tự lock D3.

## 5. Bằng chứng kiểm thử
CHƯA CHẠY. Sub-task chỉ sửa Markdown; chưa yêu cầu build/test code.

## 6. Phát hiện / vấn đề mở / rủi ro
Chưa có Jira comment xác nhận; việc này do người dùng thực hiện. Mã nguồn hiện tại có khung trường container từ SCRUM-89 nhưng chưa triển khai logic quota tương ứng.

## 7. Bước tiếp theo
Hoàn tất Sub-task 1.1, kiểm tra diff và commit `docs: Answer the 7 open questions in architecture_review_scrum64.md`. Sau đó cập nhật hash commit này vào lần cập nhật handoff của Sub-task 1.2.

## 8. Lưu ý khi tiếp tục trong chat mới
SCRUM-82 có ba sub-task và mỗi sub-task là một commit. Chỉ sửa tài liệu Markdown; không sửa kernel code. Không ghi nhận PASS nếu chưa có output kiểm thử thật.
