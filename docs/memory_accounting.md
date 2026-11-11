# SCRUM-63.6 – Memory Accounting Analysis

## 1. Mục tiêu

Phân tích cơ chế quản lý bộ nhớ trong xv6-riscv, xác định các điểm cấp phát và thu hồi tài nguyên, đồng thời xây dựng phương pháp đo bộ nhớ vật lý phục vụ Resource Benchmark.

## 2. Cơ chế quản lý bộ nhớ vật lý

Bộ cấp phát bộ nhớ trong `kernel/kalloc.c` sử dụng danh sách liên kết `kmem.freelist`. Mỗi phần tử biểu diễn một trang vật lý có kích thước 4096 byte.

Các hàm chính:

- `kinit()`: Khởi tạo bộ cấp phát bộ nhớ.
- `freerange()`: Đưa các trang vật lý hợp lệ vào danh sách trống.
- `kalloc()`: Lấy một trang từ danh sách trống.
- `kfree()`: Trả một trang về danh sách trống.
- `kfreemem()`: Đếm số trang trống và quy đổi thành số byte RAM trống.

Hàm `kfreemem()` được bổ sung trong SCRUM-63, sử dụng `kmem.lock` để bảo vệ danh sách trong quá trình duyệt.

Công thức:

**Free Memory = Free Pages × 4096 bytes**

Giá trị đo được phản ánh số byte trong danh sách trang trống của bộ cấp phát xv6, không phải tổng bộ nhớ khả dụng theo hệ thống Linux.

## 3. Cơ chế cấp phát bộ nhớ ảo

Phân tích `kernel/vm.c`:

- `uvmalloc()`: Cấp phát và ánh xạ các trang bộ nhớ vật lý.
- `uvmdealloc()`: Thu nhỏ không gian địa chỉ và thu hồi các trang không còn sử dụng.
- `uvmfree()`: Giải phóng bộ nhớ user-space và page table.
- `uvmcopy()`: Sao chép các trang đã ánh xạ từ tiến trình cha sang tiến trình con.
- `vmfault()`: Xử lý truy cập vào trang chưa ánh xạ, hỗ trợ lazy allocation.

Với lazy allocation, việc tăng kích thước vùng nhớ qua `sbrk()` không nhất thiết làm giảm ngay số trang RAM trống. Trang vật lý được cấp phát khi tiến trình thực sự truy cập vùng nhớ tương ứng.

## 4. Quản lý tài nguyên tiến trình

Trong `kernel/proc.c`:

- `kfork()`: Kiểm tra quota container, cấp phát tiến trình và sao chép vùng nhớ.
- `freeproc()`: Giải phóng các tài nguyên liên quan đến tiến trình.
- `kwait()`: Thu hồi tiến trình con đã kết thúc và gọi `freeproc()`.
- `knproc()`: Đếm số slot tiến trình có trạng thái khác `UNUSED`.

Tiến trình ở trạng thái `ZOMBIE` vẫn chiếm slot cho tới khi được thu hồi.

`knproc()` duyệt từng slot dưới spinlock riêng của tiến trình, nên kết quả không phải snapshot nguyên tử của toàn bộ bảng tiến trình.

## 5. System call đo tài nguyên

SCRUM-63 bổ sung syscall `sysinfo()` với hai trường:

- `freemem`: Số byte bộ nhớ vật lý trống.
- `nproc`: Số slot tiến trình đang sử dụng.

Syscall được đăng ký với số hiệu 28 trong phiên bản đã kiểm thử.

Dữ liệu được truyền từ kernel về user-space thông qua `copyout()`.

## 6. Kết quả thực nghiệm

### Memory Benchmark

Thử nghiệm cấp phát 32 trang, mỗi trang 4096 byte.

| Chỉ số | Kết quả |
|---|---:|
| RAM trống trước cấp phát | 133255168 bytes |
| RAM trống sau cấp phát | 133124096 bytes |
| RAM trống sau thu hồi | 133255168 bytes |
| Bộ nhớ cấp phát | 131072 bytes |
| Bộ nhớ thu hồi | 131072 bytes |
| Chênh lệch cuối bài đo | 0 bytes |

**Kết luận:** Trong lần kiểm thử này, 32 trang được cấp phát và thu hồi đầy đủ theo số liệu `sysinfo()`.

### Process Benchmark

| Chỉ số | Kết quả |
|---|---:|
| Fork attempts | 100 |
| Successful forks | 100 |
| Failed forks | 0 |
| Elapsed ticks | 2 |
| Process slots trước/sau | 3 / 3 |
| RAM trống trước/sau | 133255168 / 133255168 bytes

## 7. Thiết kế memory quota theo container

Phần này chốt cách đếm trang user cho memory quota; chưa triển khai helper hoặc thay đổi đường cấp phát trong kernel. Đơn vị quota là trang 4 KB. `max_pages == 0` nghĩa là không giới hạn. Chỉ tính trang vật lý user-space đã cấp phát thực tế; không tính page table, trapframe hoặc trang kernel.

Cấu hình quota dùng `cont_set_mem(id, max_pages)`, syscall 31; chỉ host được cấu hình container active khác 0, giá trị âm bị từ chối và không cho hạ `max_pages` thấp hơn `pages_used`. Hai helper dự kiến là `container_reserve_pages(c, n)` và `container_release_pages(c, n)`: reserve kiểm tra giới hạn và tăng bộ đếm dưới `c->lock`; release giảm bộ đếm dưới lock và panic nếu bộ đếm âm.

Không dùng `p->sz` làm số trang đã dùng. Trong `sys_sbrk()`, nhánh lazy chỉ tăng `p->sz`; trang vật lý được cấp khi `vmfault()` xử lý truy cập. `uvmunmap()` cũng bỏ qua PTE không tồn tại hoặc không valid, nên bộ đếm cần thay đổi theo leaf mapping thật sự được tạo/gỡ.

### 7.1. Điểm hook và container sở hữu

| Điểm hook | Đường gọi và container sở hữu đã xác minh | Cách đưa container vào helper | Ghi chú thu hồi/rollback |
|---|---|---|---|
| `uvmalloc()` | `growproc()` gọi cho process hiện tại khi `sys_sbrk()` chọn nhánh eager; `kexec()` gọi để cấp segment ELF và stack trên page table mới của cùng process. | Thêm tham số `struct container *` rõ ràng; truyền owner của page table từ `growproc()` hoặc `kexec()`. | Khi `kalloc()` lỗi, hàm gọi `uvmdealloc(pagetable, a, oldsz)` để gỡ phần đã map. Khi `mappages()` lỗi, trang hiện tại được `kfree()` rồi phần trước đó được rollback qua `uvmdealloc()`. Mỗi PTE hợp lệ bị gỡ phải release một reservation. |
| `vmfault()` | Gọi từ `usertrap()` khi xử lý page fault; cũng gọi từ `copyin()`, `copyout()` và `copyinstr()` nếu `walkaddr()` không tìm thấy trang. Các hàm copy nhận page table/kích thước rõ ràng và chuyển chúng vào `vmfault()`. | Đưa owner container tường minh qua call chain của trap/copy helper; không suy ra quota từ `p->sz`. Các call site hiện xử lý address space của process, kể cả page table tạm của process trong `kexec()`. | Hiện `vmfault()` gọi `kalloc()`, zero page, rồi `mappages()`; nếu cấp phát hoặc ánh xạ lỗi thì không có mapping user mới còn lại. Với quota, reserve trước `kalloc()`; nếu `kalloc()` lỗi thì release reservation, nếu `mappages()` lỗi thì `kfree()` trang và release reservation. |
| `uvmcopy()` | `kfork()` gán `np->container = p->container` trước khi gọi `uvmcopy()`; các bản sao map vào page table của child và thuộc quota child. PTE không có hoặc không valid ở page table cha đều được bỏ qua. | Thêm tham số container và truyền `np->container`; không dùng owner page table cha một cách ngầm định. | Nếu reserve/`kalloc()` thất bại, đường `err` gọi `uvmunmap(new, 0, i / PGSIZE, 1)` để dọn các trang child đã map. Nếu `mappages()` lỗi thì trang hiện tại được `kfree()` trước khi vào đường dọn. Rollback phải release quota theo owner child cho từng leaf hợp lệ. |
| `uvmunmap(..., do_free)` | Được gọi bởi `uvmdealloc()`, `uvmfree()` và đường lỗi của `uvmcopy()`. `uvmunmap()` bỏ qua page-table entry không có hoặc không valid; chỉ `do_free != 0` mới `kfree()` physical page. | Thêm/thừa kế tham số container của page table owner. Chỉ release quota user page khi `do_free != 0` và PTE leaf đang valid, cùng trang được trả về allocator. | PTE trống/lazy hole không được đếm/thu hồi. `do_free == 0` dùng cho mapping trampoline/trapframe trong proc page table, chỉ xóa mapping và không thay đổi quota. |
| `uvmdealloc()` | Được gọi khi `growproc()` thu nhỏ vùng nhớ và khi `uvmalloc()` rollback phần trước đó đã map. | Nhận container owner từ caller (`growproc()`/`uvmalloc()`) rồi truyền xuống `uvmunmap()`. | Chỉ unmap khoảng giữa `PGROUNDUP(newsz)` và `PGROUNDUP(oldsz)`; số trừ thực tế là các PTE valid được `uvmunmap()` tìm thấy, không lấy trực tiếp từ chênh lệch `p->sz`. |
| `uvmfree()` | `proc_freepagetable()` gọi `uvmfree()` sau khi gỡ mapping trampoline/trapframe; `uvmfree()` unmap user pages nếu `sz > 0`, rồi `freewalk()` giải phóng page-table pages. | Nhận owner container qua `proc_freepagetable()` và truyền tới `uvmunmap()` user. Các trang page table do `freewalk()` giải phóng không tính quota. | `proc_pagetable()` gọi `uvmfree(pagetable, 0)` khi dựng mapping trampoline lỗi; khi mapping trapframe lỗi, trước đó unmap trampoline với `do_free=0`, rồi cũng `uvmfree(..., 0)`. Chưa có user pages trên các đường lỗi này. |
| `kexec()` | Tạo page table mới cho process hiện tại, cấp trang ELF/stack trước khi commit; khi thành công đổi `p->pagetable`, rồi giải phóng image cũ. | Truyền `p->container` cho allocation/fault/free helpers của cả image mới và cũ. | Nếu lỗi, giải phóng page table mới để rollback quota và giữ image cũ. Khi thành công, image mới và cũ cùng được tính trong thời gian chuyển tiếp cho đến khi image cũ được giải phóng. |
| `freeproc()` | Khi `kwait()` reap child, caller giữ `wait_lock` và `pp->lock`; `freeproc(pp)` thu hồi page table của child, không phải process đang chạy parent. `freeproc()` hiện đặt `p->container = 0` trước `proc_freepagetable()`. | Chụp `struct container *owner = p->container` trước khi clear pointer; truyền `owner` qua `proc_freepagetable()` đến `uvmfree()`/`uvmunmap()`. Không dùng `myproc()->container`. | Trang user của child phải release về container của child. Trường hợp `allocproc()` gọi `freeproc()` khi tạo lỗi có `p->sz == 0` và chưa có user mapping. |

### 7.2. Quy tắc reserve và rollback

Mỗi trang mới phải reserve một page từ container sở hữu trước khi gọi `kalloc()`. Nếu reserve thất bại thì không cấp trang; nếu thao tác nhiều trang (như `uvmalloc()` hoặc `uvmcopy()`) đã map các trang trước đó thì phải rollback chúng qua `uvmdealloc()`/`uvmunmap()`. Nếu `kalloc()` thất bại sau reserve thì release reservation ngay. Nếu `mappages()` thất bại thì `kfree()` trang vật lý vừa lấy và release reservation của trang đó. Các trang đã map trước đó được rollback qua `uvmdealloc()` hoặc `uvmunmap(..., do_free = 1)`, nơi mỗi PTE valid được gỡ sẽ release đúng một page. Các page-table page trung gian do walk/mappages tạo không thuộc quota và được thu hồi qua đường giải phóng page table hiện có.

Với `uvmcopy()` lỗi, child container là owner của các trang copy một phần; với `kexec()` lỗi, container hiện tại vẫn là owner của image mới tạm thời và các trang ấy phải được rollback, trong khi image đang chạy không đổi. Khi `kexec()` thành công, quota phản ánh đồng thời hai image cho tới khi image cũ được giải phóng; do đó quota chặt có thể làm `exec` thất bại trong lúc image mới được dựng dù image mới có thể vừa quota sau khi image cũ bị bỏ. Việc thu hồi từ `freeproc()` luôn dùng container đã chụp từ process bị thu hồi, kể cả khi `kwait()` được gọi bởi process thuộc container khác.

Các hook kể trên chỉ đếm user data pages. `freewalk()` vẫn giải phóng page-table pages, còn mappings của trampoline/trapframe được unmap với `do_free == 0`; chúng không làm thay đổi `pages_used`.
