# Scrum84 – Container API Permission and Race Fix – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang làm

## 1. Mục tiêu và Acceptance Criteria
Áp dụng D2/D3: chỉ tiến trình thuộc host container 0 được gọi API quản trị; container không thể quay lại host; gán `p->container` được bảo vệ bằng `p->lock`. Test âm tính, ctest, roottest và chroottest phải PASS.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái (chưa làm / đã đề xuất / đã duyệt / đã commit) | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 3.1 Enforce host-only privilege | đã commit | 884655c | Dùng helper chung `container_is_host()` |
| 3.2 Block leaving a container and fix cont_enter race | đã duyệt | — | `make` PASS; reserve trước p lock; gán pointer dưới lock; release quota cũ sau |
| 3.3 Write negative test and rerun ctest | chưa làm | — | Thêm `ptest_neg`, log T13 permission |

## 3. File đã đổi (theo sub-task)
Sub-task 3.1: kernel/proc.c, kernel/defs.h, kernel/sysfile.c, kernel/sysproc.c. Dự kiến 3.2: kernel/proc.c. Dự kiến 3.3: user/ptest_neg.c, Makefile, docs/test_evidence/T13_permission_negative.txt.

## 4. Quyết định kỹ thuật và lý do
`container_is_host()` kiểm tra process hiện tại có container và container id bằng 0. `cont_info()` vẫn công khai. Khi đổi container, reserve target trước; giữ `p->lock` chỉ quanh phép gán `p->container`; nhả lock rồi release process slot cũ. Không giữ đồng thời container lock và p lock trong đoạn đổi pointer.

## 5. Bằng chứng kiểm thử
Sub-task 3.1: PASS `make`. Sub-task 3.2: PASS `make`. Chưa chạy test runtime; dự kiến log test âm tính và regression ở `docs/test_evidence/T13_permission_negative.txt`.

## 6. Phát hiện / vấn đề mở / rủi ro
SCRUM-83 đã hoàn tất; khung 8.2 có sẵn. `cont_set_cpu` và `cont_set_mem` hiện là stub trả -1; kiểm tra host-only ở đó không biến chúng thành implementation.

## 7. Bước tiếp theo
Commit 3.2 với tên `fix: Block leaving a container and fix cont_enter race`; tiếp tục 3.3. Không sửa CPU/memory quota implementation.

## 8. Lưu ý khi tiếp tục trong chat mới
SCRUM-84 phụ thuộc SCRUM-82. Mỗi sub-task một commit. Chạy QEMU với timeout, ghi PASS/FAIL theo output thật, không chạy stress vô hạn.
