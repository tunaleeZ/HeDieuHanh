# SCRUM96 — Implement PID Namespace

## Mục tiêu

Triển khai PID namespace theo thiết kế SCRUM-90, giữ global PID cho kernel và cung cấp virtual PID riêng cho mỗi container.

## Acceptance Criteria

| Tiêu chí | Bằng chứng | Trạng thái |
|---|---|---|
| Hai container độc lập nhìn thấy PID 1 | T03-02 | PASS |
| Global PID duy nhất và dùng nội bộ | T03-01; host-visible fork IDs khác nhau | PASS |
| `getpid()` trả virtual PID | T03-04; host và hai container | PASS |
| `wait()` trả PID theo namespace của parent | T03-05; nhận vpid 2 | PASS |
| `kill()` chỉ điều khiển đúng namespace | T03-06/T03-08; PID không tồn tại bị từ chối | PASS |
| Namespace-init exit đúng chính sách | T03-07; đóng namespace và reap members | PASS |
| Lock/process lifecycle an toàn | Lock-order review và QEMU 3-hart regression | PASS; chưa có stress race chuyên dụng |
| `forktest`, `ctest`, `usertests -q` không regression | `PID_namespace_regression.txt` | PASS |
| Có T03, tài liệu, log và handoff | T03 và các evidence | PASS |

## Trạng thái sub-task

| Sub-task | Trạng thái | Commit | Ghi chú |
|---|---|---|---|
| 2.1 Implement virtual PID data structures | Hoàn tất | `f3beffb` | Cấu trúc và allocator entry/fork; ctest cho thấy vpid 1,2,3; build/forktest PASS. |
| 2.2 Implement PID allocation and translation | Hoàn tất | `ec20c0d` | Helpers tra visible PID và dịch virtual PID; build, ctest, forktest PASS. |
| 2.3 Update process syscalls | Hoàn tất | `62bd5df` | getpid/wait/kill dùng vpid; entry publication atomic với PID reserve; namespace-init exit; build, ctest, forktest PASS. |
| 2.4 Test T03 and regression | Hoàn tất | `test: Test PID namespace and regression` | T03 0 failures; regression yêu cầu PASS; evidence lưu trong `docs/test_evidence/`. Hash lấy từ `git log` sau commit. |

## Thiết kế đã thống nhất

- `p->pid` là global PID, không đổi ý nghĩa và tiếp tục dùng nội bộ kernel.
- `p->virtual_pid` là PID nhìn thấy trong container. Host container 0 dùng `virtual_pid == pid` để giữ tương thích.
- `container.next_virtual_pid` cấp số tăng đơn điệu cho namespace khác 0, khởi đầu bằng 1. Process đầu tiên vào container nhận PID 1; fork cấp ID mới. Không tái sử dụng PID đã cấp.
- Giá trị `next_virtual_pid == 0` biểu thị namespace đóng hoặc cạn PID. Giới hạn PID dương là `0x7fffffff`; khi cấp PID đó, allocator đóng.
- `cont_enter()` bị từ chối nếu caller có child hiện hữu. Quan hệ parent của process nhập container vẫn giữ nguyên; host parent reap process đó bằng global PID. Regression `roottest` dùng container mới cho lần entry độc lập sau khi PID 1 cũ đã đóng namespace.
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

## 2.2 — Lookup và dịch PID

`process_visible_pid(viewer, target)` trả virtual PID khi cùng container, global PID nếu viewer thuộc host, và -1 khi container không được quan sát target. Caller phải giữ `target->lock`. `global_pid_from_visible(viewer, pid)` chuyển PID host trực tiếp và tra process đang sống trong đúng container cho viewer không-host; hàm chỉ trả global PID, không để lộ con trỏ process. `kernel/defs.h` khai báo hai helper để phần syscall dùng ở 2.3.

Đã chạy `make kernel/kernel`, `ctest`, `forktest` trong QEMU: build thành công; `ctest` báo 0 failures và quota/fork/wait hiện tại vẫn đúng; `forktest` báo `fork test OK`. Đây mới là kiểm tra helper build cùng allocator hiện hữu, chưa kiểm tra syscall đã dùng translation; phần tích hợp và race/lifecycle nằm trong 2.3–2.4.

## 2.3 — Process syscalls và namespace-init

`sys_getpid()` trả `virtual_pid`. `kwait()` vẫn duyệt theo quan hệ parent dưới `wait_lock`, giữ `pp->lock` khi lấy PID trả về qua `process_visible_pid()`. `kkill()` dịch PID người gọi sang global PID rồi tìm/đánh dấu victim dưới `p->lock`; namespace không-host chỉ tra cùng container, host giữ global PID. Zombie/slot UNUSED không còn được kill thành công.

Khi process PID 1 trong container thoát, `kexit()` đóng allocator dưới container lock và đánh dấu thành viên khác killed, đánh thức member đang ngủ. Khi process khác trong namespace thoát, `reparent()` chuyển orphan cho PID 1 còn sống; khi chính PID 1 thoát (hoặc không còn leader sống), orphan chuyển về global init. Thứ tự khóa được giữ `wait_lock -> p->lock -> container->lock`; thao tác đóng allocator thả container lock trước khi quét process table. `kcontainer_enter()` giữ process lock rồi container lock, đồng thời công bố membership và PID trước khi thả hai lock. Do đó namespace-init exit hoặc thấy process mới vào và kill nó, hoặc đóng allocator trước entry reservation khiến entry thất bại. Build `make kernel/kernel` thành công; QEMU 3 hart: `ctest` hoàn tất với 0 failures, `forktest` in `fork test OK`.

## Phát hiện và rủi ro

- `docs/process_analysis.md` còn mô tả repo trước khi container membership và CPU quota được triển khai; tài liệu này ngoài phạm vi hiện tại.
- Table `containers[]` không có destroy/reuse API. Namespace bị đóng vẫn giữ slot container suốt kernel lifetime.
- Việc `cont_enter()` từ chối process có child là hạn chế hành vi mới, được chọn để tránh quan hệ cha-con cắt qua namespace.
- `usertests -q` có nhiều tình huống PID/wait/kill ở host; host virtual PID bằng global PID nhằm giữ tương thích.

## 2.4 — T03 và regression

Test mới `user/pidns_test.c` được thêm vào `UPROGS` dưới tên `pidns_test`. Test kiểm tra PID host, hai namespace độc lập có leader PID 1, ba virtual PID con riêng biệt trong mỗi namespace, `getpid/fork/wait`, kill isolation giữa hai worker có cùng PID 2, PID không tồn tại và namespace-init exit/reap. T03 kết thúc `0 failure(s)`.

Các file được thay đổi trong SCRUM96:

| Phạm vi | File |
|---|---|
| PID data, allocator và lifecycle | `kernel/proc.h`, `kernel/proc.c`, `kernel/defs.h`, `kernel/sysproc.c` |
| Test | `user/pidns_test.c`, `user/roottest.c`, `Makefile` |
| Thiết kế và handoff | `docs/pid_namespace_design.md`, `docs/week4_handoff_files/Scrum96_handoff.md` |
| Matrix và evidence | `docs/integration_test_matrix.md`, `docs/test_evidence/T03_pid_namespace.txt`, `docs/test_evidence/PID_namespace_regression.txt` |

Build sạch `make clean && make && make fs.img` thành công (linker cảnh báo LOAD segment RWX như trước). Regression trên QEMU 3 harts: `forktest`, `ctest`, `usertests -q`, `roottest`, `chroottest /c1`, `ptest_neg`, `cputest`, `test_sysinfo` đều PASS. `roottest` cần fixture `/c1` (`setupc1`) và dùng container mới ở lần entry cuối vì namespace đầu tiên đã đóng sau khi PID 1 thoát. Evidence: `docs/test_evidence/T03_pid_namespace.txt`, `docs/test_evidence/PID_namespace_regression.txt`; ctest 0 failures, usertests `ALL TESTS PASSED`, roottest 0 failures, ptest_neg 0 failures, cputest PASS.

### Giới hạn còn lại

- Chưa chạy stress test chuyên biệt cho nhiều parent đồng thời gọi fork trong cùng namespace; allocator được bảo vệ bằng container lock, và QEMU regression chạy 3 harts.
- Container slot không tái sử dụng sau namespace-init exit do không có destroy API.
- Integration matrix còn các mục khác ngoài SCRUM96 như memory quota, bounded fork stress, multi-container quota và cleanup/reuse.

## Tổng kết và bước tiếp theo

SCRUM96 đã hoàn thành bốn sub-task và có bốn commit riêng. Không push; branch `main` sẵn sàng để người dùng tự push và cập nhật Jira. Các giới hạn còn lại được liệt kê ở trên và trong integration matrix.

## Lưu ý khi tiếp tục

Đọc handoff này và `docs/pid_namespace_design.md`; kiểm tra status, log và branch trước khi tiếp tục. Bốn commit SCRUM96: `f3beffb`, `ec20c0d`, `62bd5df`, và commit test/handoff sub-task 2.4. Không push; task yêu cầu người dùng tự push và cập nhật Jira.
