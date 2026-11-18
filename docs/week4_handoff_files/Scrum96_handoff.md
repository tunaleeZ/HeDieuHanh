# SCRUM96 — Implement PID Namespace

## Mục tiêu

Triển khai PID namespace theo thiết kế SCRUM-90, giữ global PID cho kernel và cung cấp virtual PID riêng cho mỗi container.

## Acceptance Criteria

| Tiêu chí | Bằng chứng | Trạng thái |
|---|---|---|
| Hai container độc lập nhìn thấy PID 1 | T03-02 | Chưa chạy |
| Global PID duy nhất và dùng nội bộ | T03-01, procdump/debug | Chưa xác minh đầy đủ |
| `getpid()` trả virtual PID | T03-04 | Chưa triển khai |
| `wait()` trả PID theo namespace của parent | T03-05 | Chưa triển khai |
| `kill()` chỉ điều khiển đúng namespace | T03-06/T03-08 | Chưa triển khai |
| Namespace-init exit đúng chính sách | T03-07 | Chưa triển khai |
| Lock/process lifecycle an toàn | Regression và review race | Chưa xác minh đầy đủ |
| `forktest`, `ctest`, `usertests -q` không regression | Log regression | Chưa chạy trên kernel cuối |
| Có T03, tài liệu, log và handoff | T03 và các evidence | Đang làm |

## Trạng thái sub-task

| Sub-task | Trạng thái | Commit | Ghi chú |
|---|---|---|---|
| 2.1 Implement virtual PID data structures | Hoàn tất | Điền khi cập nhật 2.2 | Cấu trúc và allocator entry/fork; ctest cho thấy vpid 1,2,3; build/forktest PASS. |
| 2.2 Implement PID allocation and translation | Chưa làm | — | Xây dựng lookup/dịch PID và kiểm tra lifecycle/race. |
| 2.3 Update process syscalls | Chưa làm | — | Cập nhật getpid, fork return, wait, kill, reparent và namespace-init. |
| 2.4 Test T03 and regression | Chưa làm | — | Tạo T03, chạy clean build và regression, lưu log. |

## Thiết kế đã thống nhất

- `p->pid` là global PID, không đổi ý nghĩa và tiếp tục dùng nội bộ kernel.
- `p->virtual_pid` là PID nhìn thấy trong container. Host container 0 dùng `virtual_pid == pid` để giữ tương thích.
- `container.next_virtual_pid` cấp số tăng đơn điệu cho namespace khác 0, khởi đầu bằng 1. Process đầu tiên vào container nhận PID 1; fork cấp ID mới. Không tái sử dụng PID đã cấp.
- Giá trị `next_virtual_pid == 0` biểu thị namespace đóng hoặc cạn PID. Giới hạn PID dương là `0x7fffffff`; khi cấp PID đó, allocator đóng.
- `cont_enter()` bị từ chối nếu caller có child hiện hữu. Quan hệ parent của process nhập container vẫn giữ nguyên; host parent reap process đó bằng global PID.
- Khi namespace-init thoát, đóng allocator, đánh dấu các thành viên khác killed, chuyển orphan sang global init để reap. Process còn lại không chạy thành namespace không có PID 1; fork/entry mới bị từ chối. Container không tái sử dụng vì không có `cont_destroy()`.
- `kill()` của process container chỉ tra cùng container; host tiếp tục nhận global PID và có chính sách quản lý toàn hệ thống hiện tại.
- `wait()` cùng namespace trả virtual PID; host parent nhận global PID cho child đã chuyển namespace.
- Lock order giữ `wait_lock -> p->lock -> container->lock`; không giữ container lock khi gọi thao tác có thể sleep. Allocator được bảo vệ bởi `container.lock`; process lookup khóa từng `p` và so sánh namespace cùng PID.

### Phương án đã cân nhắc

- **Đã chọn:** namespace-init exit đóng namespace và kill các thành viên còn lại. Cách này không để lại workload không có adopter PID 1 và không yêu cầu bầu leader mới.
- **Không chọn:** tiếp tục chạy process khi PID 1 đã thoát. Namespace không còn adopter rõ ràng và orphan/reap khó bảo đảm.
- **Không chọn:** bầu process khác làm PID 1. PID ảo đã cấp không tái sử dụng; cần thêm trạng thái leader và quy tắc đổi vai trò.
- **Không chọn:** giữ host namespace với bộ đếm riêng. Dùng global PID giữ hành vi tương thích và cho host wait/kill biểu diễn process ngoài container.
- **Không chọn:** cho process có child chuyển namespace. Điều này tạo child ở namespace khác mà PID parent không thể biểu diễn nhất quán.

## 2.1 — Cấu trúc và allocator nền

File thay đổi: `kernel/proc.h`, `kernel/proc.c`, `docs/pid_namespace_design.md`, handoff này. 2.1 bao gồm cấp phát cơ bản khi `cont_enter()`/`fork()` để kiểm tra uniqueness; 2.2 sẽ làm lookup và translation. `procdump()` hiển thị riêng global và virtual PID để kiểm chứng/debug.

Build sạch: `make clean && make && make fs.img` — thành công. Runtime trên filesystem mới, QEMU 3 harts:

```text
$ ctest
Created container ID: 1
Created child PID 2
Created child PID 3
fork returned -1 at attempt 3
=== Container Test Finished: 0 failures ===
^P
global=1 virtual=1 sleep  init container=0
global=2 virtual=2 sleep  sh container=0
global=3 virtual=1 sleep  ctest container=1
global=4 virtual=2 sleep  ctest container=1
global=5 virtual=3 sleep  ctest container=1
$ forktest
fork test
fork test OK
```

QEMU output xác nhận container ID 1 có process đầu tiên virtual PID 1 và hai child lần lượt là 2,3; global PID tương ứng là 3,4,5, không trùng nhau. `ctest` cũng đạt process limit, reap child và fork lại sau giải phóng quota. `forktest` PASS. Output build/runtime đã được thấy trực tiếp; log thử tạm nằm ngoài repo tại `/tmp/scrum96_21_qemu_final.log`.

Chính sách namespace-init đã được ghi nhận trong thiết kế nhưng phần kill/reparent/đóng namespace sẽ được triển khai ở 2.3; 2.1 chưa tuyên bố lifecycle đó đã chạy.

## Phát hiện và rủi ro

- `docs/process_analysis.md` còn mô tả repo trước khi container membership và CPU quota được triển khai; tài liệu này ngoài phạm vi hiện tại.
- Table `containers[]` không có destroy/reuse API. Namespace bị đóng vẫn giữ slot container suốt kernel lifetime.
- Việc `cont_enter()` từ chối process có child là hạn chế hành vi mới, được chọn để tránh quan hệ cha-con cắt qua namespace.
- `usertests -q` có nhiều tình huống PID/wait/kill ở host; host virtual PID bằng global PID nhằm giữ tương thích.

## Bước tiếp theo

Hoàn tất test cấu trúc/allocator trong 2.1 và commit riêng. Sau đó 2.2 triển khai namespace-aware lookup/translation; 2.3 cập nhật syscalls và xử lý namespace-init đầy đủ; 2.4 tạo T03 và chạy regression cuối.

## Lưu ý khi tiếp tục

Đọc handoff này và `docs/pid_namespace_design.md`; kiểm tra status, log và branch trước khi tiếp tục. Không push; task yêu cầu người dùng tự push và cập nhật Jira.
