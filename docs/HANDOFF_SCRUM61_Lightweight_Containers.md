# HANDOFF -- Lightweight Containers trên xv6-riscv

**Phạm vi:** Tổng hợp thiết kế, triển khai và kiểm thử SCRUM-61\
**Dự án:** Đề tài 32 -- Lightweight Containers\
**Môi trường:** xv6-riscv chạy trên QEMU\
**Trạng thái:** Các chức năng chính của SCRUM-61 đã được triển khai và
`ctest` đã chạy thành công theo log do người dùng cung cấp.\

------------------------------------------------------------------------

## 1. Bối cảnh và mục tiêu

Đây là dự án môn Hệ điều hành về **Lightweight Containers**, được triển
khai trực tiếp bên trong mã nguồn hệ điều hành xv6-riscv. Nhóm không
triển khai Docker/LXC trên Linux host. Các cơ chế container được bổ sung
vào kernel xv6 và kiểm thử khi boot xv6 trên QEMU.

SCRUM-61 xây dựng nền tảng quản lý container ở mức tiến trình:

1.  Mỗi tiến trình thuộc về một container.
2.  Tiến trình con kế thừa container của tiến trình cha khi gọi
    `fork()`.
3.  Mỗi container có thể đặt giới hạn số lượng tiến trình.
4.  Kernel từ chối tạo tiến trình mới nếu container đã đạt giới hạn.
5.  Khi tiến trình được thu hồi, quota tương ứng được giải phóng.
6.  User space có thể tạo container, chuyển tiến trình hiện tại sang
    container khác và truy vấn thông tin container thông qua system
    call.

SCRUM-62 sẽ mở rộng mô hình bằng cách cô lập thư mục gốc (root
directory) theo container.

## 2. Thiết kế tổng thể

### 2.1. Bảng container toàn cục

Kernel duy trì một bảng container tĩnh:

``` c
#define NCONTAINER 8

static struct container containers[NCONTAINER];
```

Thiết kế hiện tại có tối đa 8 container, với ID cố định theo chỉ số
trong bảng:

-   ID `0`: host container, được khởi tạo mặc định khi boot.
-   ID `1` đến `7`: container có thể được tạo thêm bằng system call.

Container 0 đại diện cho môi trường mặc định của xv6. Container 0 không
có giới hạn số tiến trình (`max_proc = 0`).

Các container hiện tại không có thao tác destroy/deactivate. Một
container đã được tạo vẫn giữ trạng thái active trong suốt thời gian
kernel chạy.

### 2.2. Cấu trúc dữ liệu

Cấu trúc container trong `kernel/proc.h`:

``` c
struct container {
  int id;
  int proc_count;
  int max_proc;
  int active;
  struct spinlock lock;
};
```

Ý nghĩa:

  Trường         Ý nghĩa
  -------------- --------------------------------------------------
  `id`           ID của container
  `proc_count`   Số tiến trình đang chiếm quota của container
  `max_proc`     Giới hạn tiến trình; `0` nghĩa là không giới hạn
  `active`       Container đã được tạo và đang hoạt động
  `lock`         Spinlock bảo vệ metadata của container

Cấu trúc thông tin trả về user space:

``` c
struct container_info {
  int id;
  int proc_count;
  int max_proc;
  int active;
};
```

`struct container_info` được khai báo tương ứng ở cả phía kernel và
`user/user.h`, với cùng thứ tự trường và kiểu dữ liệu. Đây là cấu trúc
trao đổi dữ liệu giữa kernel và user space, không phải con trỏ trực tiếp
tới `struct container`.

### 2.3. Liên kết tiến trình với container

Trong `struct proc` của `kernel/proc.h`, mỗi tiến trình có trường:

``` c
struct container *container;
```

Trường này là con trỏ tới đối tượng container trong bảng toàn cục
`containers[]`.

Quan hệ:

``` text
Kernel
 |
 +-- containers[0] : host container
 |
 +-- containers[1] : container 1
 |
 +-- containers[2] : container 2
 |
 +-- ...
 |
 +-- containers[7] : container 7

struct proc
 |
 +-- p->container ---> một phần tử trong containers[]
```

Một tiến trình chỉ thuộc về một container tại một thời điểm.

Không lưu bản sao các trường `id`, `proc_count`, `max_proc` bên trong
từng `struct proc`; tiến trình tham chiếu trực tiếp tới đối tượng
container.

## 3. Quy tắc quản lý vòng đời và quota

### 3.1. Ý nghĩa của `proc_count`

`proc_count` đếm số tiến trình đã được cấp slot trong container, bao gồm
các trạng thái:

-   `RUNNING`
-   `RUNNABLE`
-   `SLEEPING`
-   `ZOMBIE`

Một tiến trình vẫn chiếm quota trong trạng thái `ZOMBIE`, vì cấu trúc
tiến trình chưa được thu hồi hoàn toàn. Slot chỉ được trả lại khi kernel
thực sự thu hồi tiến trình thông qua `freeproc()`.

Quy tắc này giúp quota phản ánh số cấu trúc tiến trình còn được quản lý
trong container, thay vì chỉ đếm các tiến trình đang thực thi.

### 3.2. `container_reserve()`

Hàm nội bộ trong `kernel/proc.c` dùng để xin một slot:

``` c
static int
container_reserve(struct container *c)
{
  if (c == 0)
    return -1;

  acquire(&c->lock);
  if (!c->active || (c->max_proc > 0 && c->proc_count >= c->max_proc)) {
    release(&c->lock);
    return -1;
  }

  c->proc_count++;
  release(&c->lock);
  return 0;
}
```

Logic:

1.  Từ chối nếu con trỏ container không hợp lệ.
2.  Khóa metadata container.
3.  Từ chối nếu container chưa active.
4.  Nếu `max_proc > 0`, từ chối khi `proc_count >= max_proc`.
5.  Nếu còn quota, tăng `proc_count`.
6.  Nhả khóa và trả về thành công.

`max_proc = 0` là trường hợp không giới hạn, do điều kiện giới hạn chỉ
áp dụng khi `max_proc > 0`.

### 3.3. `container_release()`

Hàm nội bộ giải phóng một slot:

-   Khóa container.
-   Giảm `proc_count`.
-   Kiểm tra lỗi underflow; nếu số đếm âm thì kernel panic.
-   Nhả khóa.

Mọi đường đi giải phóng tiến trình phải tránh giải phóng quota hai lần.

### 3.4. Khi tạo tiến trình

#### Tiến trình init

Trong `userinit()`:

1.  Xin một slot từ host container 0.
2.  Gán `p->container` trỏ tới host container.
3.  Khởi tạo tiến trình init như cơ chế xv6 thông thường.

Vì vậy, host container có ít nhất một tiến trình sau khi init được tạo.

#### `fork()` / `kfork()`

Trong `kfork()`:

1.  Lấy container của tiến trình cha.
2.  Gọi `container_reserve()` trước khi tạo tiến trình con.
3.  Nếu hết quota, trả về lỗi ngay; không tạo tiến trình con.
4.  Gọi `allocproc()`.
5.  Nếu `allocproc()` thất bại, trả lại slot đã reserve.
6.  Gán `np->container = p->container`.
7.  Sao chép address space bằng `uvmcopy()`.
8.  Nếu `uvmcopy()` thất bại, đường rollback phải trả quota và xóa liên
    kết container trước khi kết thúc.
9.  Nếu thành công, tiến trình con trở thành `RUNNABLE`.

Như vậy, tiến trình con luôn kế thừa container của cha.

#### Thu hồi tiến trình

Trong `freeproc()`:

1.  Nếu tiến trình còn gắn với container, gọi `container_release()`.
2.  Xóa con trỏ container.
3.  Tiếp tục reset các trường của `struct proc` theo cơ chế xv6.

`kwait()` gọi `freeproc()` khi thu hồi tiến trình con ở trạng thái
`ZOMBIE`. Vì vậy, tiến trình zombie vẫn chiếm quota cho tới lúc được
parent `wait()` và kernel thu hồi.

### 3.5. Chuyển container bằng `cont_enter()`

`cont_enter(id)` chỉ chuyển **tiến trình gọi system call** sang
container đích. Nó không tự động chuyển các tiến trình con đang tồn tại.

Thứ tự xử lý được thiết kế để tránh mất slot hoặc làm thay đổi container
khi target đã hết quota:

1.  Kiểm tra ID hợp lệ và container đích đang active.
2.  Lấy tiến trình hiện tại bằng `myproc()`.
3.  Nếu đã ở container đích thì không cần thay đổi.
4.  Xin slot trong container đích bằng `container_reserve()`.
5.  Nếu xin slot thất bại, trả lỗi và giữ nguyên container cũ.
6.  Nếu thành công, cập nhật `p->container`.
7.  Giải phóng slot của container cũ.

Không giữ đồng thời lock của hai container trong thao tác chuyển đổi.

## 4. Khởi tạo container

Trong `procinit()`:

-   Khởi tạo spinlock cho từng phần tử trong bảng container.
-   Gán ID tương ứng với chỉ số.
-   Khởi tạo `proc_count = 0`.
-   Khởi tạo `max_proc = 0`.
-   Đặt trạng thái ban đầu inactive.
-   Kích hoạt container 0 và giữ `max_proc = 0` để host không bị giới
    hạn.

Khi tạo container mới, kernel tìm ID từ `1` đến `NCONTAINER - 1`, khóa
từng container và chọn phần tử chưa active đầu tiên. Container được kích
hoạt với `proc_count = 0` và `max_proc` theo tham số người dùng.

Các tham số `max_proc` âm bị từ chối.

## 5. System call interface

SCRUM-61 cung cấp ba system call.

### 5.1. `cont_create(int max_proc)`

Tạo container mới.

-   `max_proc < 0`: trả `-1`.
-   `max_proc = 0`: không giới hạn tiến trình.
-   `max_proc > 0`: giới hạn số tiến trình.
-   Thành công: trả ID container mới.
-   Hết slot trong bảng: trả `-1`.

### 5.2. `cont_enter(int id)`

Chuyển tiến trình hiện tại sang container có ID được chỉ định.

-   ID không hợp lệ hoặc container inactive: trả `-1`.
-   Không đủ quota ở container đích: trả `-1`, tiến trình vẫn ở
    container cũ.
-   Thành công: trả `0`.

### 5.3. `cont_info(int id, struct container_info *info)`

Lấy thông tin container và chép dữ liệu từ kernel sang user space.

-   ID không hợp lệ hoặc container inactive: trả `-1`.
-   Địa chỉ user không hợp lệ hoặc `copyout()` thất bại: trả `-1`.
-   Thành công: trả `0` và điền các trường `id`, `proc_count`,
    `max_proc`, `active`.

Wrapper `sys_cont_info()` cần dùng đúng chữ ký `copyout()` của phiên bản
xv6 trong repository. Phiên bản hiện tại đã được kiểm tra có dạng:

``` c
copyout(p->pagetable, p->sz, addr, (char *)&info, sizeof(info))
```

### 5.4. Các điểm đăng ký system call

Các thành phần liên quan:

-   `kernel/syscall.h`: số hiệu `SYS_cont_create`, `SYS_cont_enter`,
    `SYS_cont_info`.
-   `kernel/syscall.c`: khai báo `extern` và ánh xạ các số hiệu vào
    `syscalls[]`.
-   `kernel/sysproc.c`: các wrapper `sys_cont_create()`,
    `sys_cont_enter()`, `sys_cont_info()`.
-   `user/user.h`: khai báo hàm user API và `struct container_info`.
-   `user/usys.pl`: sinh stub user-space cho ba system call.
-   `kernel/defs.h`: khai báo các hàm kernel `kcontainer_create()`,
    `kcontainer_enter()`, `kcontainer_info()`.

Các số hiệu syscall được dùng trong phiên bản hiện tại:

``` c
#define SYS_cont_create 24
#define SYS_cont_enter  25
#define SYS_cont_info   26
```

## 6. Danh sách file đã tham gia SCRUM-61

  -----------------------------------------------------------------------
  File/                                Vai trò
  ----------------------------------- -----------------------------------
  `kernel/proc.h`                     Định nghĩa `struct container`,
                                      `struct container_info`, thêm con
                                      trỏ container vào `struct proc`

  `kernel/proc.c`                     Bảng container, lock,
                                      reserve/release, khởi tạo, fork,
                                      cleanup, create/enter/info

  `kernel/defs.h`                     Prototype các hàm kernel

  `kernel/syscall.h`                  Số hiệu syscall

  `kernel/syscall.c`                  Đăng ký syscall

  `kernel/sysproc.c`                  Wrapper system call

  `user/user.h`                       User API và cấu trúc thông tin

  `user/usys.pl`                      Stub syscall phía user

  `user/ctest.c`                      Chương trình kiểm thử container

  `Makefile`                          Thêm `_ctest` vào danh sách chương
                                      trình user
  -----------------------------------------------------------------------

Lưu ý: danh sách trên phản ánh các file đã được đề cập và triển khai
trong quá trình làm việc. Trước khi tạo commit hoặc chỉnh sửa tiếp, cần
dùng `git status` để xác nhận trạng thái thực tế của repository.

## 7. Kiểm thử SCRUM-61

Chương trình `ctest` đã được chạy trong xv6. Log người dùng cung cấp:

``` text
=== Container Test ===

[Test 1] Host container
Container 0: count=3, max=0, active=1

[Test 2] Create container
Created container ID: 1
Container 1: count=0, max=3, active=1

[Test 3] Invalid container limit
PASS: negative limit rejected

[Test 4] Enter container
Container 1: count=1, max=3, active=1

[Test 5] Process quota
Created child PID 4
Created child PID 5
fork returned -1 at attempt 3
Container 1: count=3, max=3, active=1

[Test 6] Reap children
Child reaped
Child reaped
Container 1: count=1, max=3, active=1

[Test 7] Fork after quota release
PASS: fork succeeded after quota release
Container 1: count=1, max=3, active=1

=== Container Test Finished ===
```


-   Host container active, không giới hạn quota (`max=0`).
-   Tạo được container ID 1 với quota 3.
-   Giới hạn âm bị từ chối.
-   Tiến trình kiểm thử chuyển vào container 1 và chiếm 1 slot.
-   Hai tiến trình con được tạo; lần fork thứ ba thất bại vì đã đạt
    quota 3.
-   Sau khi hai child được `wait()` và thu hồi, `proc_count` giảm từ 3
    về 1.
-   Fork tiếp theo thành công sau khi quota được giải phóng.

Kết quả này kiểm tra được các luồng chính: tạo container, enter, quota
khi fork, thu hồi quota và khả năng fork lại. Đây là kiểm thử chức năng
theo kịch bản; chưa phải kiểm thử concurrency/stress hoặc kiểm thử bảo
mật.

Trước đó, `forktest` của xv6 cũng đã in `forktest OK`. `make clean` và
`make qemu` đã chạy thành công trong giai đoạn triển khai trước đó.


## 8. Các giới hạn hiện tại và lưu ý kỹ thuật

1.  Bảng container có kích thước cố định `NCONTAINER = 8`.
2.  Không có system call hủy container.
3.  Không có quyền/capability kiểm soát tiến trình nào được phép gọi
    `cont_create()` hoặc `cont_enter()`.
4.  `cont_enter()` chỉ chuyển tiến trình hiện tại; không chuyển cả
    process tree.
5.  `proc_count` tính cả zombie cho đến lúc `freeproc()`.
6.  `container.lock` bảo vệ metadata container. Các thao tác thay đổi
    quota phải tiếp tục đi qua
    `container_reserve()`/`container_release()`.
7.  `procdump()` có thể hiển thị container ID để debug; đây không phải
    API quản lý container cho user space.
8.  Kiểm thử hiện tại xác nhận các đường đi cơ bản, chưa chứng minh đầy
    đủ tính đúng đắn dưới tải đồng thời trên nhiều CPU.

## 9. Trạng thái SCRUM-61 và bước tiếp theo

SCRUM-61: các chức năng cốt lõi đã được triển khai và chương trình
`ctest` cho kết quả thành công theo log. Có thể cập nhật Jira sau khi
kiểm tra trạng thái commit và lưu bằng chứng.

### SCRUM-62 -- Per-Container Root Directory

Mục tiêu: mỗi tiến trình có thể sử dụng một root directory riêng, tương
tự khái niệm `chroot` ở hệ điều hành Unix, nhưng được quản lý theo mô
hình container trong xv6.

Các điểm cần khảo sát trong source:

-   `kernel/fs.c`: `namei()`, `namex()`, `nameiparent()`, `skipelem()`.
-   `kernel/proc.h`: `struct proc`, trường inode root mới dự kiến.
-   `kernel/proc.c`: `kfork()` kế thừa root và `freeproc()` giải phóng
    tham chiếu inode.
-   `kernel/sysfile.c`: các syscall thao tác file và thư mục, đặc biệt
    các đường dẫn tuyệt đối/tương đối.
-   `kernel/file.c` và `kernel/fs.c`: `idup()`, `iput()`, `begin_op()`,
    `end_op()` và quy tắc quản lý tham chiếu inode.
-   `kernel/syscall.c`, `kernel/syscall.h`, `kernel/sysproc.c` hoặc
    `sysfile.c`, `user/user.h`, `user/usys.pl`: nơi đăng ký syscall
    `chroot` theo thiết kế cuối cùng.


Các yêu cầu thiết kế cần giải quyết trước khi code:

1.  Root directory được lưu ở đâu: mỗi process hay mỗi container? Để
    đúng ngữ nghĩa container và tránh các process cùng container có root
    khác nhau ngoài ý muốn, nên cân nhắc lưu root trong
    `struct container`, sau đó các process tra cứu thông qua
    `p->container`. Nếu bài toán nhóm yêu cầu root theo từng process thì
    phải ghi rõ khác biệt.
2.  `fork()` phải giữ tham chiếu inode root đúng cách.
3.  `exec()`, `open()`, `mkdir()`, `chdir()`, `link()`, `unlink()` và
    các thao tác pathname khác phải tuân theo root của tiến trình.
4.  Đường dẫn tuyệt đối phải bắt đầu từ container root thay vì global
    `ROOTDEV/ROOTINO`.
5.  Đường dẫn có `..` không được phép thoát khỏi container root.
6.  `chroot` phải xác thực inode đích, quản lý reference đúng cách và
    tránh rò rỉ inode.
7.  Phải quyết định root ban đầu của container mới: có thể là root
    filesystem mặc định cho đến khi gọi `chroot`, hoặc root được chỉ
    định trong lúc tạo container. Không tự chọn mà không thống nhất với
    nhóm.

**Việc đầu tiên trong cuộc trò chuyện tiếp theo:** yêu cầu người dùng
gửi các file `kernel/fs.c`, `kernel/proc.h`, `kernel/proc.c`,
`kernel/sysfile.c`, `kernel/file.c` (hoặc các phần liên quan) để khảo
sát chính xác phiên bản xv6 hiện tại. Không viết code SCRUM-62 trước khi
đã xem implementation thực tế của `namex()`/`namei()` và cơ chế inode
reference.
