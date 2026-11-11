# Scrum82 – Resolve Open Design Questions for Week 3 – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: xong chờ review

## 1. Mục tiêu và Acceptance Criteria
Chốt bảy câu hỏi trong mục 7 của `docs/architecture_review_scrum64.md` theo các quyết định D1–D6; hoàn tất API và tham số trong `docs/cpu_quota_design.md`; bổ sung thiết kế quota vào `docs/memory_accounting.md` sau khi đối chiếu kernel/vm.c, proc.c và exec.c. Chỉ sửa Markdown. Người dùng sẽ tự xác nhận quyết định trên Jira.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái (chưa làm / đã đề xuất / đã duyệt / đã commit) | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 1.1 Answer the 7 open questions in architecture_review_scrum64.md | đã commit | c1584af | Đã có mục 7, bảng syscall và quy ước lock |
| 1.2 Finalize cpu_quota_design.md | đã commit | 0c4557d | Chốt API/tham số, charge/reset, giả định và rủi ro; giữ CPU-01..CPU-09 |
| 1.3 Add quota design section to memory_accounting.md | đã commit | — | Bảng hook đã rà vm.c, proc.c, exec.c; commit cuối của task nên hash được xem trong git log |

## 3. File đã đổi (theo sub-task)
Sub-task 1.1: `docs/architecture_review_scrum64.md`, `docs/Scrum82_handoff.md`.
Sub-task 1.2: `docs/cpu_quota_design.md`, `docs/Scrum82_handoff.md`.
Sub-task 1.3: `docs/memory_accounting.md`, `docs/Scrum82_handoff.md`.

## 4. Quyết định kỹ thuật và lý do
Áp dụng quyết định D1–D6 đã chốt trong yêu cầu SCRUM-82. Sub-task 1.1 ghi cả Quyet dinh, Ly do, He qua cho từng câu hỏi, cùng các số syscall 28–31 và thứ tự lock D3. Sub-task 1.2 chốt quota soft, chu kỳ mặc định 20, quota 0 là unlimited, host miễn throttle, API cont_set_cpu syscall 30, charge tại usertrap/kerneltrap và reset khi scheduler gọi container_cpu_allowed(). Sub-task 1.3 xác định các hook memory quota, truyền owner container tường minh và rollback theo các đường cấp phát/giải phóng đã có.

## 5. Bằng chứng kiểm thử
CHƯA CHẠY build/test code; đây là các sub-task tài liệu. `git diff --check` được chạy trước mỗi commit. Sub-task 1.3 dựa trên việc đọc kernel/vm.c, kernel/proc.c và kernel/exec.c.

## 6. Phát hiện / vấn đề mở / rủi ro
Chưa có Jira comment xác nhận; việc này do người dùng thực hiện. Mã nguồn hiện tại có khung trường container từ SCRUM-89 nhưng chưa triển khai logic quota tương ứng. Các giả định về độ phân giải tick, đọc ticks không khóa, timer drift và sleeplock khi throttle cần kiểm chứng trong SCRUM-85. Memory quota chưa được gắn vào các đường cấp phát; thiết kế đề xuất truyền container sở hữu tường minh và cần được hiện thực trong task memory tiếp theo.

## 7. Bước tiếp theo
SCRUM-82 đã hoàn tất với ba commit riêng; hash 1.1 là c1584af, 1.2 là 0c4557d, commit 1.3 là commit cuối cùng trong `git log`. Người dùng tự đăng Jira comment xác nhận quyết định.

## 8. Lưu ý khi tiếp tục trong chat mới
SCRUM-82 có ba sub-task và mỗi sub-task là một commit. Chỉ sửa tài liệu Markdown; không sửa kernel code. Không ghi nhận PASS nếu chưa có output kiểm thử thật.
