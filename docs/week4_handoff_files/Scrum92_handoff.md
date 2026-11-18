# SCRUM92 — Container Root Isolation

## Mục tiêu và Acceptance Criteria

Khắc phục F1: không cho tiến trình vào container chưa cấu hình root rồi tiếp tục phân giải đường dẫn qua root/cwd của host.

| Acceptance Criteria | Bằng chứng cần có | Trạng thái hiện tại |
|---|---|---|
| `cont_enter()` không cho truy cập host filesystem trái chính sách cô lập | Test âm tính root-unset và root `/c1` | Chưa triển khai |
| Container chưa cấu hình root được xử lý an toàn | `cont_enter()` trả `-1`, `proc_count` không đổi | Chưa triển khai |
| `roottest` và `ptest_neg` PASS | Log hồi quy cuối | Chưa chạy sau sửa |
| Không regression fork, exec, chdir | `forktest`, `roottest`, `chroottest /c1` | Chưa chạy sau sửa |
| Có commit, test log và handoff trên GitHub | Các commit sub-task và log; push do người dùng thực hiện | Đang làm |

## Trạng thái sub-task

| Sub-task | Trạng thái | Commit hash | Ghi chú |
|---|---|---|---|
| 1.1 Survey root-unset bug | Hoàn tất | Điền khi cập nhật sub-task 1.2 | Khảo sát code và tái hiện runtime F1; chính sách A và gộp F10 đã được duyệt. |
| 1.2 Implement fix | Chưa làm | — | Chặn vào container chưa có root; cập nhật các test cũ cần root `/`. |
| 1.3 Add negative tests | Chưa làm | — | Kiểm tra root-unset, proc_count, F10 và ranh giới root `/c1`; lưu log T13. |
| 1.4 Run regression and handoff | Chưa làm | — | Build sạch, regression, cập nhật ma trận và defect, đối chiếu log tuần 3. |

## 1.1 — Khảo sát

### Luồng hiện tại và inode reference

`sys_cont_enter()` ở `kernel/sysproc.c` đọc ID rồi gọi `kcontainer_enter()` trong `kernel/proc.c`. `sys_cont_setroot()` ở `kernel/sysfile.c` gọi `namei()` trong transaction, xác minh inode là thư mục, rồi chuyển reference trả về từ `namei()` cho `kcontainer_setroot()`. Nếu kernel từ chối, syscall gọi `iput()` reference đó.

| Nhánh `kcontainer_enter()` hiện tại | Hành vi | Inode reference |
|---|---|---|
| `old == new` (vào lại cùng container) | Trả `0` trước reserve/root snapshot | Không thêm hay thả reference root/cwd; quota không đổi. |
| `new->root == 0` | Reserve quota, đổi `p->container`, giải phóng quota cũ; không đổi root/cwd | Không thêm hay thả inode reference. Tiến trình giữ nguyên root/cwd host: F1. |
| `new->root != 0` | Snapshot root dưới `new->lock`; `idup(root)`, đặt root mới và `cwd = idup(root)`, rồi `iput()` cwd/root cũ trong transaction | Container giữ reference nhận từ `namei()` khi setroot; tiến trình nhận thêm hai reference riêng. Root của tiến trình và cwd mỗi trường giữ một reference. |

`userinit()` tạo cwd bằng `namei("/")` và thêm một reference cho root bằng `idup()`. `kfork()` thêm reference độc lập cho cả `p->cwd` và `p->root`; `kexit()` thả cả hai trong transaction. Nhánh root đã cấu hình trong `kcontainer_enter()` cân bằng hai reference cũ bằng `iput()` và giữ hai reference mới. Nhánh root chưa cấu hình không leak inode nhưng để lộ quyền phân giải path của host. Nhánh vào lại cùng container không áp dụng lại root; với chính sách root bất biến, đây là no-op hợp lệ.

`namex()` hiện khởi đầu absolute path từ `p->root`, relative path từ `p->cwd`, và chặn `..` khi inode hiện tại chính là root tiến trình. Vì vậy không cần sửa `kernel/fs.c` cho F1: lỗi nằm ở việc cho phép chuyển container mà không có root mới.

### Tái hiện F1 trên runtime

Probe tạm `user/f1probe.c` tạo `/outside.txt` ở host, tạo container chưa đặt root, fork, cho child gọi `cont_enter(id)`, rồi mở và đọc `/outside.txt`. Probe đã được gỡ sau khi chạy; không còn trong Makefile hay source tree.

Lệnh build: `make fs.img` — hoàn tất thành công.

Lệnh khởi chạy: `make qemu`; tại shell xv6 chạy `f1probe`.

Output thật:

```text
$ f1probe
F1 REPRODUCED: container opened host /outside.txt
$
```

Kết quả xác nhận child trong container chưa cấu hình root mở được file host. Đây là tái hiện trực tiếp F1 trên HEAD trước khi sửa.

### Rà lời gọi cont_enter hiện có

| File | Vị trí | Đã đặt root trước khi vào? | Hành động dự kiến |
|---|---:|---|---|
| `user/ctest.c` | 80 | Chưa | Thêm `cont_setroot(id, "/")` rõ ràng và kiểm tra kết quả. |
| `user/cputest.c` | 41 | Chưa | Đặt root `/` cho target trước khi worker vào. |
| `user/ptest_neg.c` | 45 | Chưa | Đổi thành ca từ chối root-unset; sau đó set `/` trước ca kiểm tra quyền một chiều. |
| `user/roottest.c` | 272 | Có: `/c1` | Giữ ca hiện có, bổ sung kiểm tra boundary nếu phù hợp với sub-task 1.3. |

Không có lời gọi `cont_enter()` trong `user/bench_*.c` hoặc `user/crun.c`; `crun.c` không tồn tại trong repo.

### Quyết định chính sách

Người dùng đã duyệt phương án A: `cont_enter(id)` trả `-1` nếu container đích chưa cấu hình root. Kiểm tra root và reserve proc slot phải phối hợp dưới `container->lock`: nếu root rỗng thì trả lỗi trước khi thay đổi quota hoặc `p->container`; nếu reserve thành công thì `proc_count > 0` khiến setroot đồng thời bị từ chối. Không giữ container lock khi `iput()`, `begin_op()`, cấp phát hay yield. Việc gán `p->container` tiếp tục nằm dưới `p->lock` theo quy ước hiện tại.

Người dùng cũng duyệt gộp F10: `cont_setroot()` từ chối `id <= 0`. Setter chỉ thành công khi container đích active, root chưa đặt và `proc_count == 0`. Root chỉ được cấu hình một lần; không cho phép đặt root sau khi có process trong container. Các test chỉ cần process quota hoặc CPU quota sẽ đặt `/` rõ ràng trước `cont_enter()` để thể hiện lựa chọn không cô lập filesystem.

Phương án B (root mặc định `/` khi tạo container) bị loại vì container mới mặc định có quyền truy cập filesystem host, trái với fail-closed của F1. Phương án C (cho vào rồi đổi root) bị loại vì process có thể chạy và truy cập filesystem trước khi root được áp dụng; đồng thời setroot sau khi process đã vào xung đột với quy tắc root chỉ đặt một lần và mở race giữa path lookup với thay root.

File dự kiến đổi ở 1.2: `kernel/proc.c`, `user/ctest.c`, `user/cputest.c`, `user/ptest_neg.c`. File dự kiến đổi ở 1.3: `user/ptest_neg.c`, có thể `user/roottest.c`, `docs/test_evidence/T13_f1_enforcement.txt`.

## Phát hiện và rủi ro

- F1 tái hiện được và chưa sửa ở sub-task 1.1.
- Root inode container vẫn được giữ đến hết vòng đời kernel vì hiện chưa có `cont_destroy`; đây là giới hạn đã nêu ở F11, ngoài phạm vi task.
- Chưa kiểm thử stress inode hoặc race đồng thời ngoài phân tích khóa; các sub-task sau tập trung kiểm tra hành vi và quota như acceptance criteria.
- DEFECT-01 có thể ảnh hưởng nhập lệnh QEMU; chạy từng lệnh một và giữ output thật.

## Bước tiếp theo

1.2 sửa `kcontainer_enter()` và `kcontainer_setroot()`, đồng thời cập nhật các test cũ phụ thuộc hành vi vào container chưa có root. Sau đó build sạch và chạy bộ kiểm tra nêu trong prompt. Tiếp tục 1.3 thêm negative tests và lưu log thật; 1.4 chạy toàn bộ regression, đối chiếu log tuần 3, cập nhật ma trận/defect và handoff.

## Lưu ý khi tiếp tục trong chat mới

Đọc file này, chạy `git status` và `git log --oneline -10`, rồi tóm tắt trạng thái trước khi tiếp tục. Không push; task yêu cầu kết thúc bằng danh sách commit và nhắc người dùng tự push/cập nhật Jira.
