# Review tuần 3: SCRUM-82 đến SCRUM-87

Ngày review: 2026-11-11
Commit được review: `cb8ac8b`
Phạm vi: 20 commit từ `7e43876` đến `cb8ac8b`
Cách review: đọc code và tài liệu; build/test trong worktree tạm; không sửa working tree chính.

## 1. Kết luận chung

[ĐÃ XÁC MINH] Cả 20 commit build được bằng `make kernel/kernel` và `make fs.img`. Regression trên HEAD: `forktest` báo `fork test OK`, `usertests -q` báo `ALL TESTS PASSED`.

Chưa nên coi toàn bộ SCRUM-82–87 là hoàn tất. `cont_enter()` cho phép vào container chưa đặt root, giữ lại root/cwd cũ và có thể làm lộ đường dẫn host. CPU quota chỉ charge ở timer interrupt nên có thể bỏ sót CPU giữa các tick; throttle cũng có thể kéo dài lúc tiến trình giữ sleeplock.

Không sửa file trong working tree chính. Build/QEMU chạy trong worktree tạm; worktree đã xóa. Log QEMU: `/tmp/review-qemu-head.log`.

## 2. Verdict theo task

- **SCRUM-82 — Đạt có lưu ý.** Có đủ nội dung thiết kế, nhưng `cpu_quota_design.md` vẫn nói accounting/enforcement chưa triển khai dù code hiện đã có; kết luận cuối `architecture_review_scrum64.md` cũng là trạng thái cũ.
- **SCRUM-83 — Chưa đạt.** Root được áp dụng nếu đã cấu hình, nhưng vào container chưa có root vẫn giữ root/cwd cũ. Test chỉ kiểm tra trường hợp đã đặt root trước khi vào.
- **SCRUM-84 — Đạt có lưu ý.** Quyền host-only, một chiều và assignment của `p->container` dưới `p->lock` đã có; test âm tính và `ctest` qua. Container chưa có root vẫn có thể giữ root host.
- **SCRUM-85 — Đạt có lưu ý.** Hai lượt `cputest` qua và host tiếp tục tiến triển; quota theo tick có giới hạn đã nêu ở F2, throttle có thể giữ sleeplock ở F3.
- **SCRUM-86 — Đạt có lưu ý.** Chỉ là nền tảng cấu hình, chưa nối VM hooks đúng phạm vi. Test xác nhận đặt/đọc `max_pages`, nhưng không gọi helper reserve/release; helper có phép cộng `int` có thể tràn.
- **SCRUM-87 — Đạt có lưu ý.** Năm benchmark có tham số và thống kê nhất quán. Commit 6.1 không sửa kernel; phần điền CPU fields đã có từ `7e43876`.

## 3. Phát hiện

### F1 — Blocker: container chưa đặt root vẫn dùng root/cwd cũ

Vị trí: `kernel/proc.c`, `kcontainer_enter()`, dòng 1019–1048; `kcontainer_setroot()`, dòng 1053–1076.

`kcontainer_enter()` đổi `p->container`; nếu `new->root == 0`, không cập nhật `p->root` hoặc `p->cwd`. `cont_setroot()` đặt root container nhưng không cập nhật tiến trình đã vào trước đó. Vì vậy tiến trình có thể thuộc container mới nhưng vẫn phân giải path qua root host.

Tác động: phá ranh giới filesystem. `ptest_neg` vào container chưa đặt root nhưng không thử mở file host.

Đề xuất: từ chối `cont_enter()` khi root chưa cấu hình, hoặc bảo đảm không thể chạy tiến trình trong container cho tới khi root đã được áp dụng. Chưa sửa. Liên quan H1, H8, H10.

### F2 — Major: quota CPU không tính đầy đủ các đoạn chạy ngắn hơn timer tick

Vị trí: `kernel/trap.c`, `usertrap()` dòng 84–89 và `kerneltrap()` dòng 159–165; `kernel/proc.c`, `container_charge_tick()` dòng 951–960.

Accounting chỉ cộng một tick tại timer interrupt. Các đoạn chạy rồi block/yield trước timer tiếp theo không bị charge. Đây là giới hạn của đơn vị quota đã chọn, nhưng tải gồm nhiều đoạn chạy ngắn có thể dùng CPU mà quota không phản ánh đầy đủ.

Đề xuất: đánh giá accounting theo elapsed runtime hoặc ghi rõ giới hạn/điều kiện sử dụng quota. Chưa sửa. Liên quan H6.

### F3 — Major: throttle có thể kéo dài lúc tiến trình giữ sleeplock

Vị trí: `kernel/trap.c`, `kerneltrap()` dòng 159–165; `kernel/proc.c`, `scheduler()` dòng 589–618.

Timer có thể yield tiến trình đang chạy trong kernel; scheduler sau đó bỏ qua tiến trình đã hết quota. Nếu tiến trình đang giữ inode/pipe/log sleeplock, tiến trình khác cần lock đó phải chờ tới khi tiến trình bị throttle được lập lịch lại. Rủi ro đã được nêu trong thiết kế CPU, nhưng chưa có test riêng.

Đề xuất: kiểm thử tương tác lock và cân nhắc cách xử lý tiến trình giữ lock khi quota cạn. Chưa sửa. Liên quan H11.

### F4 — Minor: phép cộng trong memory reserve có thể tràn `int`

Vị trí: `kernel/proc.c`, `container_reserve_pages()`, dòng 72–84.

Điều kiện `pages_used + n > max_pages` cộng hai số `int`. Nếu tổng tràn, giới hạn có thể bị vượt. Helper chưa được nối vào VM và chưa có syscall gọi trực tiếp.

Đề xuất: kiểm tra bằng phép trừ có điều kiện hoặc kiểu rộng trước khi cộng; kiểm thử helper trước khi nối VM hooks. Chưa sửa. Liên quan H14.

### F5 — Minor: mẫu `ticks` có thể cũ hơn `period_start`

Vị trí: `kernel/proc.c`, `kcontainer_set_cpu()` dòng 916–927 và `container_cpu_allowed()` dòng 972–985.

Cả hai hàm lấy mẫu `ticks` trước khi lấy `container->lock`. Nếu scheduler lấy mẫu trước, chờ khóa, rồi `set_cpu` ghi `period_start` mới hơn, phép trừ unsigned trong `container_cpu_allowed()` wrap và có thể reset quota sớm một lần.

Đây là suy luận từ thứ tự khóa, chưa tái hiện runtime. Đề xuất: xử lý trường hợp `now < period_start` rõ ràng. Chưa sửa. Liên quan H5.

### F6 — Minor: fixture `/c1` không idempotent

Vị trí: `user/setupc1.c`, dòng 62–80.

`mkdir("/c1")` thất bại nếu thư mục đã tồn tại, khiến script không chạy lặp lại trên cùng filesystem. Chạy lần đầu trên filesystem mới đã PASS.

Đề xuất: cho phép thư mục có sẵn sau khi xác minh đúng loại hoặc dọn fixture cũ trước khi dựng. Chưa sửa. Liên quan AC SCRUM-83.

### F7 — Minor: tài liệu CPU quota còn trạng thái cũ

Vị trí: `docs/cpu_quota_design.md`, mục 2 và mục 10; `docs/week2_handoff_files/architecture_review_scrum64.md`, kết luận mục 8.

Tài liệu nói accounting/scheduler enforcement chưa triển khai, nhưng hiện code có `container_charge_tick()`, `container_cpu_allowed()` và scheduler enforcement. Mô tả API cuối dùng `cont_set_cpu`, không còn tên syscall cũ.

Đề xuất: ghi rõ đây là snapshot thiết kế trước SCRUM-85 hoặc cập nhật trạng thái hiện tại. Chưa sửa. Liên quan H14.

### F8 — Minor: handoff lỗi thời/thiếu hash và commit benchmark chạm handoff task khác

Vị trí: `docs/week3_handoff_files/Scrum89_handoff.md`, mục 2 và 7; handoff Scrum82/83/84/87; commit `cb8ac8b`.

Scrum89 vẫn ghi 8.2 chờ commit/hash `—` dù `7e43876` đã có. Một số handoff Scrum82/83/84/87 để hash cuối trống hoặc ghi “xem trong git log”. `cb8ac8b` sửa handoff Scrum85 và Scrum86 dù commit chỉ mô tả benchmark Scrum87.

Đề xuất: cập nhật trạng thái/hash và cân nhắc tách commit handoff khỏi benchmark nếu cần giữ nguyên tắc một sub-task mỗi commit. Chưa sửa. Liên quan H15.

### F9 — Minor: định dạng clang-format khác trên các dòng mới

`git clang-format --diff b48ec18` báo khác biệt trong `kernel/proc.c`, `kernel/proc.h`, `kernel/syscall.c`, `user/cputest.c`, `user/ctest.c`, `user/ptest_neg.c`. Không có khác biệt mới ở `kernel/syscall.h`/`kernel/sysproc.c`.

Đề xuất: áp dụng clang-format cho các dòng mới trong commit riêng nếu quy ước repo yêu cầu. Chưa sửa.

### F10 — Info: `cont_setroot(0, ...)` được chấp nhận

Vị trí: `kernel/proc.c`, `kcontainer_setroot()`, dòng 1063–1075.

ID 0 được chấp nhận, nhưng setter chỉ giữ reference root trong container; root host của tiến trình hiện tại không đổi. Vì không có destroy path, reference này giữ tới hết vòng đời kernel. Chưa thấy ghi riêng trường hợp ID 0. Liên quan H8, H9.

### F11 — Info: root inode container được giữ tới hết vòng đời

Không có `cont_destroy`, nên root reference không được `iput`. Handoff Scrum83 đã ghi đây là giới hạn có chủ ý hiện tại. Liên quan H9.

## 4. Giả thuyết H1–H16

| H | Kết quả | Căn cứ |
|---|---|---|
| H1 | Bác bỏ | Container chưa đặt root giữ root/cwd cũ sau `cont_enter()`; F1. |
| H2 | Xác nhận | Các nhánh `sys_cont_setroot`, `kcontainer_enter`, `kfork`, `kexit` cân bằng reference theo ownership; chưa chạy stress inode riêng. |
| H3 | Xác nhận có lưu ý | Không thấy đường giữ container lock rồi lấy `p->lock`; `procdump()` đọc `p->container` không khóa để in chẩn đoán. |
| H4 | Xác nhận theo code, chưa test cô lập | Throttle nhả `p->lock`, không đặt `found`; scheduler tới `wfi`. Timer được cấu hình trên mọi hart. Chưa ép tất cả runnable cùng hết quota. |
| H5 | Xác nhận khả năng, chưa tái hiện | Mẫu ticks cũ hơn `period_start` có thể wrap unsigned; dự kiến reset sớm một lần. |
| H6 | Xác nhận | Chỉ timer interrupt được charge; đoạn chạy ngắn hơn tick có thể không bị tính. |
| H7 | Bác bỏ | `container_info` có layout 48 byte, không có padding; tất cả field được gán trước copyout. |
| H8 | Xác nhận có lưu ý | `cont_setroot(0, ...)` được chấp nhận; process đã vào container trước lúc root đặt không được cập nhật. |
| H9 | Xác nhận | Không có `cont_destroy`; root reference giữ suốt vòng đời, đã ghi trong Scrum83 handoff. |
| H10 | Bác bỏ nếu hiểu là mọi truy cập | Path lookup trong root đã cấu hình chặn `..` và outside paths; FD mở trước chroot vẫn dùng được. Root-unset cũng giữ root host. |
| H11 | Xác nhận | Timer yield có thể xảy ra khi giữ sleeplock; scheduler có thể throttle tiến trình đó. Đã ghi rủi ro nhưng chưa test. |
| H12 | Chưa kết luận nguyên nhân chênh lệch | Benchmark source chỉ có commit `56ab566`; Makefile flags không đổi giữa baseline và HEAD. Raw run cũ thiếu duration/tổng iterations. |
| H13 | Xác nhận có lưu ý | `cputest` có điều kiện FAIL; hai lượt review cho khoảng 23% so với host worker, gần mức 5/20. Cửa sổ đo xấp xỉ theo uptime. |
| H14 | Bác bỏ một phần | Memory survey nhìn chung khớp code và ghi rõ chưa nối hooks; CPU docs còn trạng thái cũ. |
| H15 | Xác nhận | Commit 6.1 không đổi kernel; `cb8ac8b` sửa handoff nhiều task; Scrum89 stale; tag không có trên remote. |
| H16 | Bác bỏ | Tự chạy `forktest` và `usertests -q`; lần lượt `fork test OK`, `ALL TESTS PASSED`. |

## 5. Acceptance Criteria

| Task | Bằng chứng | Trạng thái |
|---|---|---|
| 82 | Mục 7 có 7 đáp án; API CPU và thiết kế memory có. CPU docs còn trạng thái cũ. | Một phần |
| 83 | `setupc1`, `roottest` PASS; path boundary đúng khi root đã đặt; root-unset lộ root host; fixture không idempotent. | Chưa đạt |
| 84 | `ptest_neg` và `ctest` 0 failures; assignment dưới `p->lock`; admin checks có. | Đạt có lưu ý |
| 85 | Hai `cputest` PASS, host tiến triển, quota worker thấp hơn; chưa test lock-holder/kill/all-throttle riêng. | Một phần |
| 86 | Hook survey, helper và `cont_set_mem`; test đặt/đọc `max_pages`; VM hooks chưa nối đúng phạm vi. | Một phần |
| 87 | `cont_info` đọc fields dưới lock; năm benchmark và thống kê đúng; commit 6.1 không sửa kernel. | Đạt có lưu ý |

## 6. Build và tự chạy test

Cả 20 commit được checkout riêng trong worktree tạm; tại mỗi commit chạy `make clean`, `make kernel/kernel`, `make fs.img`. Tất cả đều thành công. Baseline `b48ec18` và HEAD có cùng hai linker warning:

- `kernel/kernel has a LOAD segment with RWX permissions`
- `user/_forktest has a LOAD segment with RWX permissions`

Build HEAD sạch bằng `make clean && make && make fs.img` cũng thành công. QEMU test chạy từ shell xv6 sau boot: `qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic` với virtio disk `fs.img` theo cấu hình Makefile. Cột Scrum/sub-task dưới đây ghi task implementation được kiểm tra; “Regression” nghĩa là kiểm thử tương thích, không phải sub-task đã tạo chương trình test đó.

| Lệnh test trong xv6 / build host | Chức năng được kiểm tra | Scrum / sub-task liên quan | Kết quả thực tế |
|---|---|---|---|
| Mỗi commit: `make clean`; `make kernel/kernel`; `make fs.img` | Build kernel và filesystem tại đúng commit | Tất cả 20 commit SCRUM-82–87; khung nền SCRUM-89/8.2 | Cả 20 commit thành công; chỉ có hai linker warning nêu trên, giống baseline. |
| HEAD: `make clean && make && make fs.img` | Build sạch trước khi chạy QEMU | Regression HEAD cho SCRUM-82–87 | Thành công; warning giống baseline. |
| `ptest_neg` | Host-only admin calls bị từ chối trong container; `cont_info` vẫn công khai; kiểm tra một chiều | SCRUM-84/3.1–3.3 (đặc biệt test 3.3) | PASS, 0 failures. Test vào container chưa đặt root nhưng không thử mở đường dẫn host. |
| `ctest` | Process limit, giải phóng slot, `cont_set_mem` đặt/đọc `max_pages`, từ chối giá trị âm và setter trong container | SCRUM-84/3.3; SCRUM-86/5.2 | PASS, 0 failures. Không trực tiếp gọi helper page reserve/release. |
| `setupc1` | Tạo `/c1`, file trong root, file ngoài root và executable fixture | SCRUM-83/2.4 | PASS lần đầu trên filesystem mới. Không idempotent nếu chạy lại trên cùng filesystem. |
| `roottest` | `chroot`, absolute/relative path, `..`, FD mở trước chroot, fork/exec, link/unlink và `cont_enter` với root đã đặt | SCRUM-83/2.3–2.4 | PASS, 0 failures. Không test `cont_enter` trước khi đặt container root. |
| `chroottest /c1` | Chroot và đổi cwd tới root mới | Regression chroot cho SCRUM-83; test chương trình có từ trước | In `chroot succeeded`, `chdir to new root succeeded`; shell nhận lại prompt. |
| `cputest` ×2 | Quota 5/20, host progress, worker progress theo cửa sổ, đọc CPU fields qua `cont_info` | SCRUM-85/4.5; SCRUM-87/6.1 | Cả hai PASS, `zero_windows=0`. Lượt 1: A 276195000/B 1199097000 iterations; lượt 2: A 273484000/B 1203708000. |
| `test_sysinfo` | Syscall sysinfo legacy: free memory và process slots | Regression syscall 28; có trong kiểm tra nền SCRUM-89/8.2, ngoài SCRUM-82–87 | PASS. |
| `bench_cpu` | CPU benchmark 100 ticks | SCRUM-87/6.2 | 643390 iterations/100 ticks trong lượt review. |
| `bench_mem` | Cấp/thu hồi 32 trang và so sánh free memory | SCRUM-87/6.2 | 131072 bytes cấp phát và thu hồi. |
| `bench_proc` | 100 fork/wait, process slots và free memory trước/sau | SCRUM-87/6.2 | 100/100 fork; slots 3 trước/sau; PASS. |
| `forktest` | Regression fork/exit/wait sau scheduler và trap changes | SCRUM-85/4.2, 4.4; regression chung | `fork test OK`. |
| `usertests -q` | Regression suite xv6, gồm process, filesystem, memory/lazy allocation và preemption | SCRUM-85/4.2, 4.4; regression chung | `ALL TESTS PASSED`. Log có các `usertrap(): unexpected scause` ở ca truy cập địa chỉ không hợp lệ; suite tiếp tục và kết thúc PASS. |

Benchmark raw logs `benchmark_run_1..5.txt` tự tính lại: CPU `[618954, 617294, 598448, 585704, 589970]`, mean **602074**, population SD **13742.01**. Process elapsed ticks `[0,0,2,1,0]`, mean **0.6**, population SD **0.8**.

`docs/test_evidence/scrum65_qemu.log` chỉ có root/chroot tests, không có benchmark. Các root tests tương ứng đều qua ở HEAD.

[CHƯA CHẠY] Stress inode > `NINODE`, lặp quota/process count và kill process đang throttle; các thử nghiệm này cần bạn duyệt trước. Cũng chưa tái hiện runtime root-unset leak hoặc race H5. Chưa chạy riêng từng test trong một boot riêng; bảng trên ghi chuỗi lệnh chạy trên cùng một boot QEMU sạch.

## 7. Hành động ưu tiên

1. Chặn `cont_enter()` vào container chưa đặt root hoặc bảo đảm root được áp dụng trước khi process chạy.
2. Đánh giá accounting cho tải ngắn hơn timer tick.
3. Kiểm tra throttle khi giữ sleeplock.
4. Đồng bộ trạng thái tài liệu/handoff và hash.
5. Sửa overflow memory helper trước khi nối VM hooks; làm fixture idempotent.
