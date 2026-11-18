# SCRUM92 — Container Root Isolation

## Mục tiêu và Acceptance Criteria

Khắc phục F1: không cho tiến trình vào container chưa cấu hình root rồi tiếp tục phân giải đường dẫn qua root/cwd của host.

| Acceptance Criteria | Bằng chứng cần có | Trạng thái hiện tại |
|---|---|---|
| `cont_enter()` không cho truy cập host filesystem trái chính sách cô lập | Test âm tính root-unset và root `/c1` | Đạt; chờ regression cuối |
| Container chưa cấu hình root được xử lý an toàn | `cont_enter()` trả `-1`, `proc_count` không đổi | Đạt; kiểm tra trong `ptest_neg` |
| `roottest` và `ptest_neg` PASS | `docs/test_evidence/T13_f1_enforcement.txt` | Đạt |
| Không regression fork, exec, chdir | `forktest`, `roottest`, `chroottest /c1` ở 1.2; regression cuối còn chờ | Đạt có lưu ý; cần lặp ở 1.4 |
| Có commit, test log và handoff trên GitHub | Commit local và log; push do người dùng thực hiện | Đang làm; chưa push |

## Trạng thái sub-task

| Sub-task | Trạng thái | Commit hash | Ghi chú |
|---|---|---|---|
| 1.1 Survey root-unset bug | Hoàn tất | `acb5e50` | Khảo sát code và tái hiện runtime F1; chính sách A và gộp F10 đã được duyệt. |
| 1.2 Implement fix | Hoàn tất | Điền khi cập nhật sub-task 1.3 | Kernel từ chối root-unset và F10; test cũ đặt `/` rõ ràng. Build sạch và regression mục tiêu PASS. |
| 1.3 Add negative tests | Hoàn tất | Điền khi cập nhật sub-task 1.4 | `roottest` và `ptest_neg` PASS; log T13 đã lưu. |
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

### 1.2 — Triển khai fix và kiểm tra

`kcontainer_enter()` hiện kiểm tra `active`, root và process quota dưới `new->lock`, đồng thời reserve slot trước khi nhả lock. Root rỗng hoặc quota đầy trả `-1` mà không đổi `proc_count` hay `p->container`. Việc reserve cùng khóa với `kcontainer_setroot()` loại race: setroot chỉ thành công khi `proc_count == 0`; root không thể được đặt sau khi enter đã reserve slot. Sau khi nhả container lock, kernel `idup()` root, thay root/cwd, rồi thả hai inode reference cũ trong transaction. `kcontainer_setroot()` từ chối ID 0, container không active, root đã đặt và container có process.

Các test cũ `ctest`, `cputest`, `ptest_neg` đặt root `/` rõ ràng trước `cont_enter()` để tiếp tục kiểm tra process quota, CPU quota và quyền quản trị mà không dựa vào hành vi root-unset.

Build chạy: `make clean && make && make fs.img` — hoàn tất thành công.

Lệnh runtime tại shell xv6, chạy từng lệnh một trên boot sạch:

```text
$ setupc1
PASS: /c1 root fixture ready
$ roottest
=== ROOT TEST FINISHED: 0 failures ===
$ chroottest /c1
Changing root to: /c1
chroot succeeded
chdir to new root succeeded
$ ctest
=== Container Test Finished: 0 failures ===
$ ptest_neg
=== PERMISSION TEST FINISHED: 0 failures ===
$ cputest
RESULT: container A iterations=215939000 zero_windows=0
RESULT: host B iterations=971168000
=== CPU QUOTA TEST PASS ===
$ forktest
fork test
fork test OK
```

`roottest` cũng in PASS cho root boundary, `..`, fork child root, `chdir ..` ở root, exec trong root và từ chối exec ngoài root; toàn bộ kết thúc 0 failures. `ctest` xác nhận process quota 3, từ chối fork khi đủ quota, thu hồi slot sau wait và fork thành công lại. `ptest_neg` xác nhận host-only administration và chuyển container một chiều. Đây là output thật từ QEMU; build không lưu log riêng trong sub-task 1.2.

### 1.3 — Negative tests

`user/ptest_neg.c` xác nhận entry vào container chưa có root trả `-1` và `proc_count` không đổi; xác nhận `cont_setroot(0, "/")` thất bại với path hợp lệ; sau đó đặt `/` cho ca kiểm tra quyền quản trị.

`user/roottest.c` thêm child bắt đầu với cwd host `/`, vào container có root `/c1`, rồi kiểm tra mở được `/test.txt`, bị chặn `/outside.txt`, `../outside.txt` và relative `outside.txt`, `chdir("..")` ở root vẫn nằm trong root, và host cha vẫn mở được `/outside.txt`. Parent đồng bộ khi child đang ở trong container, xác nhận `proc_count > 0` và gọi `cont_setroot()` với ID/path hợp lệ để kiểm tra setter từ chối khi process đang chạy.

Lệnh, fixture và output QEMU đầy đủ: [`docs/test_evidence/T13_f1_enforcement.txt`](../test_evidence/T13_f1_enforcement.txt). Build sạch, `setupc1`, `roottest` và `ptest_neg` đều hoàn tất; hai test kết thúc 0 failures.

Giới hạn kiểm thử: theo phương án A, không thể có process hợp lệ trong container khi root còn unset; sau khi đặt root, quy tắc “chỉ đặt một lần” cũng làm `cont_setroot` thất bại. Vì vậy test runtime xác nhận lời gọi thất bại khi `proc_count > 0`, nhưng không thể phân biệt điều kiện `proc_count > 0` với điều kiện root đã được đặt. `kcontainer_setroot()` có kiểm tra cả hai điều kiện; riêng proc_count được xác minh thêm bằng code review.

## Phát hiện và rủi ro

- F1 tái hiện được và chưa sửa ở sub-task 1.1.
- Root inode container vẫn được giữ đến hết vòng đời kernel vì hiện chưa có `cont_destroy`; đây là giới hạn đã nêu ở F11, ngoài phạm vi task.
- Chưa kiểm thử stress inode hoặc race đồng thời ngoài phân tích khóa; các sub-task sau tập trung kiểm tra hành vi và quota như acceptance criteria.
- DEFECT-01 có thể ảnh hưởng nhập lệnh QEMU; chạy từng lệnh một và giữ output thật.

## Bước tiếp theo

1.2 sửa `kcontainer_enter()` và `kcontainer_setroot()`, đồng thời cập nhật các test cũ phụ thuộc hành vi vào container chưa có root. Sau đó build sạch và chạy bộ kiểm tra nêu trong prompt. Tiếp tục 1.3 thêm negative tests và lưu log thật; 1.4 chạy toàn bộ regression, đối chiếu log tuần 3, cập nhật ma trận/defect và handoff.

## Lưu ý khi tiếp tục trong chat mới

Đọc file này, chạy `git status` và `git log --oneline -10`, rồi tóm tắt trạng thái trước khi tiếp tục. Không push; task yêu cầu kết thúc bằng danh sách commit và nhắc người dùng tự push/cập nhật Jira.
