# HANDOFF -- Path Boundary & Chroot Isolation trên xv6-riscv

**Phạm vi:** Tổng hợp thực trạng công việc SCRUM-62 đã triển khai trong repo hiện tại  
**Dự án:** Đề tài 32 -- Lightweight Containers  
**Môi trường:** xv6-riscv + QEMU  
**Trạng thái:** Đã triển khai trong mã nguồn hiện tại; cần người review kiểm tra trước khi merge hoặc cập nhật Jira.

------------------------------------------------------------------------

## 1. Mục tiêu SCRUM-62

SCRUM-62 tập trung vào việc giới hạn truy cập filesystem theo root của tiến trình, nhằm ngăn tiến trình vượt ra khỏi “container root” bằng các đường dẫn tuyệt đối hoặc `..`.

Các trọng tâm chính:

1. Gắn một inode root riêng cho từng tiến trình (`struct proc->root`).
2. Dùng `p->root` thay cho `ROOTINO` khi xử lý đường dẫn bắt đầu bằng `/`.
3. Chặn escape root khi đang ở process root và gặp component `..`.
4. Bổ sung syscall `chroot(path)` để đổi root cho tiến trình hiện tại.
5. Đảm bảo `cwd` và `root` được kế thừa/giải phóng đúng trong `fork()` và `exit()`.
6. Đưa test path-boundary vào user-space để validate hành vi.

------------------------------------------------------------------------

## 2. Chứng cứ Git và lịch sử thay đổi

Các commit liên quan đến SCRUM-62 đã xuất hiện trong `git log` như sau:

- `defafd6` -- feat: Add per-process root inode to struct proc
- `549be9d` -- feat: Make absolute path resolution start at the process root
- `030864c` -- feat: Block parent-directory escape at the container root
- `438af34` -- feat: Add chroot-style syscall
- `4ea3d3c` -- feat: Write path-boundary test program

Repo hiện tại có trạng thái sạch (`git status --short` rỗng) và build đang ổn định (`make -j4` báo `kernel/kernel is up to date`).

------------------------------------------------------------------------

## 3. Các nhiệm vụ đã hoàn thành

### 62.1 · Review path resolution and syscall entry points

Đã rà soát các điểm dependency theo yêu cầu:

- `kernel/fs.c`: `skipelem()`, `namex()`, `namei()`, `nameiparent()`
- `kernel/sysfile.c`: `sys_open()`, `sys_chdir()`, `sys_chroot()`
- `kernel/exec.c`: `kexec()` dùng `namei()` khi exec binary
- `kernel/proc.c`: khởi tạo và quản lý `cwd`, `root`

Các điểm phụ thuộc vào root toàn cục / cwd đã được xác định và phản ánh trong `docs/filesystem_analysis.md`.

### 62.2 · Add per-process root inode to struct proc

Đã thực hiện trong `kernel/proc.h` và `kernel/proc.c`:

- Thêm `struct inode *root;` vào `struct proc`.
- Trong `userinit()`, sau khi khởi tạo `cwd = namei("/")`, gán `p->root = idup(p->cwd);`.
- Trong `kfork()`, kế thừa `np->root = idup(p->root);`.
- Trong `kexit()`, gọi `begin_op(); iput(p->cwd); iput(p->root); end_op();` trước khi reset con trỏ.

Điểm cần lưu ý: phần này đồng bộ với logic kế thừa `cwd` và có transaction boundary đúng như yêu cầu.

### 62.3 · Make absolute path resolution start at the process root

Đã sửa ở `kernel/fs.c` trong `namex()`:

- Nếu đường dẫn bắt đầu bằng `/`:
  - dùng `p->root` khi có tiến trình hiện tại,
  - fallback về `ROOTINO` khi bootstrap chưa có `p`.
- Relative path tiếp tục dùng `p->cwd`.

Thay đổi chính ở vị trí:

```c
if (*path == '/') {
  if (p == 0 || p->root == 0)
    ip = iget(ROOTDEV, ROOTINO);
  else
    ip = idup(p->root);
} else {
  if (p == 0)
    panic("namex: no current process");
  ip = idup(p->cwd);
}
```

### 62.4 · Block parent-directory escape at the container root

Đã bổ sung guard trong `namex()` để chặn `..` vượt root:

- Nếu đang ở chính inode root của process (`dev` và `inum` khớp `p->root`),
- và component hiện tại là `..`,
- thì kernel giữ nguyên inode hiện tại thay vì traverse lên cha.

Điểm đặc biệt cần kiểm tra:

- `chdir("..")` khi đang ở root
- `a/../../..`
- `/../..`
- `nameiparent()` với `..` ở cuối

Đây là phần quan trọng nhất để ngăn escape path.

### 62.5 · Add chroot-style syscall

Đã thêm syscall mới `chroot(path)`.

Sửa đổi chính:

- `kernel/syscall.h`: `#define SYS_chroot 27`
- `kernel/syscall.c`: đăng ký handler `sys_chroot`
- `kernel/sysfile.c`: triển khai `sys_chroot(void)`
- `user/user.h`: khai báo `int chroot(const char *);`
- `user/usys.pl`: export syscall cho user-space

Logic của `sys_chroot()`:

1. Resolve path bằng `namei(path)`.
2. Kiểm tra là directory.
3. Lưu `oldroot` và `oldcwd`.
4. Gán `p->root = ip; p->cwd = idup(ip);`
5. `iput(oldcwd); iput(oldroot);` trong transaction.

### 62.6 · Prepare a test root filesystem

Ban đầu mục tiêu này là tạo một root filesystem riêng cho kiểm thử (ví dụ `/c1`) với binary tối thiểu và file đánh dấu ngoài root. Trong repo hiện tại, phần này được thể hiện qua:

- `user/roottest.c` thực hiện chroot tới `/c1`
- `user/chroottest.c` là quick smoke test cho `chroot`
- `Makefile` đã thêm `_roottest` và `_chroottest` vào `UPROGS`

Lưu ý: không có symlink trong xv6, nên việc test thực tế dựa trên `chroot()` và `open()` trên filesystem hiện có, không phải tạo rootfs mới bằng file hệ thống phức tạp.

### 62.7 · Write path-boundary test program

Đã thêm `user/roottest.c`.

Các tình huống đang test:

- Open đường dẫn tuyệt đối hợp lệ trong root mới.
- Open đường dẫn tương đối hợp lệ.
- `..` không được phép escape root.
- `/../..` không vượt khỏi root.
- `chdir("..")` ở root phải giữ nguyên vị trí.
- Exec binary nằm trong root thành công.
- Exec binary nằm ngoài root bị chặn.
- Mở file đánh dấu ngoài root phải thất bại.

### 62.8 · Run tests, collect evidence and update Jira

Đã có evidence gần đây:

- `git log` cho thấy các commit tương ứng với từng nhiệm vụ đã được merge vào `main`.
- `make -j4` thực hiện thành công: `make: 'kernel/kernel' is up to date.`
- `user/roottest.c` được compile vào `UPROGS` và sẵn sàng cho test runtime trên QEMU.

Chưa có cập nhật Jira được lưu trong repo; phần này cần người review/maintainer thực hiện trên Jira hoặc tracking tool ngoài repo.

------------------------------------------------------------------------

## 4. Các file chính đã đổi

| File | Vai trò |
|---|---|
| `kernel/proc.h` | Thêm `struct inode *root` vào `struct proc` |
| `kernel/proc.c` | Khởi tạo, kế thừa và giải phóng `root` |
| `kernel/fs.c` | root-aware pathname resolution và guard `..` |
| `kernel/sysfile.c` | triển khai `sys_chroot()` và cập nhật `cwd`|
| `kernel/syscall.h` | định nghĩa syscall number cho `chroot` |
| `kernel/syscall.c` | đăng ký syscall |
| `user/user.h` | khai báo user-space API |
| `user/usys.pl` | export syscall |
| `user/chroottest.c` | smoke test cho `chroot` |
| `user/roottest.c` | kiểm thử path boundary đầy đủ |
| `Makefile` | thêm `roottest` và `chroottest` vào `UPROGS` |
| `docs/filesystem_analysis.md` | cập nhật phân tích và trạng thái hiện tại |

------------------------------------------------------------------------

## 5. Ghi chú khi review

1. Mục tiêu của SCRUM-62 đã được triển khai trong kernel hiện tại; tài liệu này là handoff để review, không phải bản thiết kế mới.
2. `namex()` đang là điểm tập trung của toàn bộ boundary logic; nếu cần mở rộng, cần kiểm tra thêm các đường dẫn truyền qua `create()`, `link()`, `unlink()` và `exec()`.
3. Với `sys_chroot()`, việc đặt `cwd` về `root` cần được xem lại từ góc độ quy trình và các tiến trình con sau khi `fork()`.
4. Việc cập nhật Jira / báo cáo kiểm thử formal vẫn cần được thực hiện bên ngoài repo hoặc trong hệ thống tracking của nhóm.

------------------------------------------------------------------------

## 6. Hướng review tiếp theo

- Đọc lại `docs/filesystem_analysis.md` để đảm bảo mô tả khớp thực tế hiện tại.
- Kiểm tra `user/roottest.c` với QEMU theo từng trường hợp `PASS/FAIL`.
- Nếu cần, bổ sung một bộ rootfs mẫu cho `/c1` và file đánh dấu ngoài root cho kiểm thử tự động hóa hơn.
- Sau khi review xong, tạo Jira task hoặc PR note cho SCRUM-62 và gắn commit hash thực tế.
