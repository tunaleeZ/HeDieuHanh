# Hướng Dẫn Sử Dụng Công Cụ `container-run` Trên xv6-riscv

## 1. Giới thiệu & Mục đích

`container-run` là công cụ dòng lệnh (CLI utility) trong không gian người dùng (user-space) của hệ điều hành xv6-riscv. Công cụ cho phép người quản trị:
- Tạo mới và khởi chạy một chương trình cô lập hoàn toàn bên trong một lightweight container chỉ bằng một dòng lệnh từ xv6 shell.
- Áp đặt các giới hạn tài nguyên đa chiều: số lượng tiến trình (`-p`), thời gian CPU (`-c`), và dung lượng bộ nhớ vật lý (`-m`).
- Bảo đảm an toàn tuyệt đối cho hệ thống host nhờ quy trình kiểm tra pre-flight và thứ tự thiết lập cô lập nghiêm ngặt.
- Quan sát trạng thái tài nguyên và vòng đời container (`-v`, `-l`, `-i`).

---

## 2. Yêu cầu Vận hành & Môi trường

1. **Vị trí thực thi:** `container-run` bắt buộc phải được chạy từ **container 0 (Host shell)**. Nếu chạy từ bên trong một container khác, lệnh sẽ bị từ chối do không có quyền quản trị.
2. **Môi trường Fixture (`/c1`):**
   - Trước khi chạy container với thư mục gốc `/c1`, cần tạo môi trường bằng lệnh:
     ```text
     $ setupc1
     ```
   - Lệnh `setupc1` tạo thư mục `/c1`, file `/c1/test.txt`, binary `/c1/echo`, và file kiểm tra ranh giới `/outside.txt` trên host.
   - Để chạy các chương trình khác trong container, sao chép binary từ host vào `/c1` bằng tiện ích `copyfile`:
     ```text
     $ copyfile /cat /c1/cat
     $ copyfile /ls /c1/ls
     $ copyfile /bench_proc /c1/bench_proc
     $ copyfile /bench_cpu /c1/bench_cpu
     $ copyfile /bench_mem /c1/bench_mem
     ```

---

## 3. Cú pháp & Bảng Tùy chọn

### 3.1. Cú pháp lệnh

1. **Khởi tạo và chạy chương trình trong container mới:**
   ```text
   container-run [-p max_proc] [-c quota:period] [-m max_pages] [-v] <root> <cmd> [args...]
   ```

2. **Chạy thêm chương trình trong container đã tạo sẵn:**
   ```text
   container-run -i <id> [-v] <cmd> [args...]
   ```

3. **Liệt kê trạng thái các slot container:**
   ```text
   container-run -l
   ```

### 3.2. Bảng tùy chọn chi tiết

| Cờ / Tham số | Kiểu dữ liệu | Mặc định | Ý nghĩa & Quy tắc |
|---|---|---|---|
| `-p <max_proc>` | Số nguyên | 0 (vô hạn) | Giới hạn số tiến trình tối đa được phép tồn tại đồng thời trong container. Lưu ý: tiến trình chính cũng tính là 1 process. |
| `-c <quota>:<period>` | Cặp số nguyên | Không giới hạn | Giới hạn thời gian CPU (ticks) trên chu kỳ. Ràng buộc: $0 \le quota \le period$, $period > 0$. $quota = 0$ nghĩa là **không giới hạn CPU**. |
| `-m <max_pages>` | Số nguyên | 0 (vô hạn) | Giới hạn số trang RAM vật lý tối đa (mỗi trang 4096 bytes). |
| `-v` | Cờ bật | Tắt | Chế độ Verbose: in thông báo khi tạo container và thống kê tài nguyên sau khi tiến trình kết thúc. |
| `-i <id>` | Số nguyên (1..7) | Không | Thực thi tiếp trong container `<id>` đang active. Cấm dùng chung với `-p`, `-c`, `-m`, hoặc `<root>`. |
| `-l` | Cờ độc lập | Không | In bảng trạng thái 8 slot container qua `cont_info` rồi thoát ngay. |
| `<root>` | Đường dẫn | Bắt buộc | Thư mục trên host filesystem làm root cho container (ví dụ `/c1`). |
| `<cmd>` | Đường dẫn | Bắt buộc | Chương trình thực thi tính từ root container (ví dụ `/echo`, `/ls`). |
| `[args...]` | Danh sách chuỗi | Rỗng | Các đối số truyền nguyên bản cho chương trình con. |

---

## 4. Quy trình Cấu hình An toàn 7 Bước (0 – 6)

Để bảo đảm không làm hỏng shell host, không rò rỉ quyền hạn và không lãng phí 7 slot container quý giá khi tham số sai, `container-run` tuân thủ nghiêm ngặt quy trình:

1. **Bước 0 (Kiểm tra đối số):** Parse tham số phía user-space. Kiểm tra cú pháp, $quota \le period$, số đối số $\le 31$.
2. **Bước 1 (Pre-flight check phía Host):** Dùng `open` và `fstat` kiểm tra `<root>` tồn tại và là thư mục (`T_DIR`), kiểm tra `<root>/<cmd>` tồn tại và là file (`T_FILE`). Nếu sai, báo lỗi và **không gọi `cont_create`**, tránh mất slot container.
3. **Bước 2 (Khởi tạo container):** Gọi `cont_create(max_proc)`. Nhận `cid` (1..7).
4. **Bước 3 (Thiết lập quota tài nguyên):** Gọi `cont_set_mem` và `cont_set_cpu` cấu hình RAM và CPU.
5. **Bước 4 (Thiết lập Root Isolation):** Gọi `cont_setroot(cid, root)`. Bảo đảm root được cố định an toàn trước khi có bất kỳ tiến trình nào bước vào.
6. **Bước 5 (Khởi tạo tiến trình qua `fork`):**
   - **Tiến trình con:** Gọi `cont_enter(cid)`. Nếu lỗi $\rightarrow$ thoát mã 1. Nếu thành công $\rightarrow$ `exec(cmd, args)`. Nếu `exec` lỗi $\rightarrow$ thoát mã 127.
   - **Tiến trình cha (Host):** Tuyệt đối **không gọi `cont_enter`** để giữ nguyên quyền host.
7. **Bước 6 (Thu hồi & Báo cáo):** Cha gọi `wait(&status)`. In thống kê tài nguyên nếu có `-v` và trả về mã trạng thái tương ứng.

---

## 5. Các Ví Dụ Demo Từng Bước (Kèm Output Thực Nghiệm)

### 5.1. Chạy chương trình cơ bản
```text
$ container-run /c1 /echo hello
hello
```

### 5.2. Ngăn ngừa lãng phí slot khi đường dẫn sai (Pre-flight Check)
```text
$ container-run /no_such_dir /echo
container-run: root '/no_such_dir' does not exist

$ container-run /c1 /no_such_cmd
container-run: command '/no_such_cmd' not found in root '/c1'

$ container-run -l
ID  ACTIVE  PROCS/MAX  CPU_QUOTA/PERIOD  MEM_PAGES/MAX
0   YES     3/0        0/20             14/0
1   YES     0/0        0/20             0/0
```
*(Slot container không bị tạo lãng phí).*

### 5.3. Giới hạn số lượng tiến trình (Process Quota)
```text
$ container-run -p 1 /c1 /bench_proc
=== PROCESS BENCHMARK ===
Fork attempts: 100
Successful forks: 0
Failed forks: 100
Elapsed ticks: 1
CHECK: process benchmark
```
*(Tiến trình chính chiếm quota `max_proc = 1`, toàn bộ 100 lần `fork` bên trong container bị chặn đứng).*

### 5.4. Giới hạn tỷ lệ CPU (CPU Quota Rate Limiting)
```text
$ bench_cpu
=== CPU BENCHMARK ===
Elapsed ticks: 100
Iterations: 601906
Iterations/tick: 6019

$ container-run -c 5:20 /c1 /bench_cpu
=== CPU BENCHMARK ===
Elapsed ticks: 101
Iterations: 157059
Iterations/tick: 1555
```
*(Tỷ lệ $1555 / 6019 \approx 25.8\%$, bám sát cấu hình CPU quota 5/20 = 25%).*

### 5.5. Giới hạn bộ nhớ vật lý (Memory Quota Enforcement)
```text
$ container-run -v -m 8 /c1 /bench_mem
container-run: container 4 created
container-run: exec '/bench_mem' failed in container 4
container-run: container 4 finished (proc_count=0, cpu_used=0, pages_used=0)
```
*(Binary `bench_mem` có dung lượng ~36 KB, đòi hỏi hơn 9 trang bộ nhớ nên bị kernel chặn ngay tại `exec` khi đặt `max_pages = 8`).*

### 5.6. Cô lập hệ thống tệp (Filesystem Isolation)
```text
$ cat /outside.txt
secret

$ container-run /c1 /cat /outside.txt
cat: cannot open /outside.txt
```
*(Tiến trình trong container bị giam hoàn toàn trong `/c1`, không thể truy cập `/outside.txt` của host).*

---

## 6. Bảng Mã Thoát & Thông Báo Lỗi

| Mã thoát | Nguyên nhân |
|---|---|
| `0` | Lệnh trong container chạy thành công và trả về 0; hoặc lệnh `-l` hoàn tất. |
| `1` | Lỗi cú pháp CLI, pre-flight check thất bại, hoặc syscall cấu hình container lỗi. |
| `127` | Lệnh không thực thi được trong container (`exec` thất bại). |
| `N` | Mã lỗi do chính chương trình trong container gọi `exit(N)`. |

---

## 7. Giới Hạn Hệ Thống & Xử Lý Sự Cố

1. **Giới hạn 7 container:** Kernel xv6 chỉ hỗ trợ 8 slot container (slot 0 là host, 1..7 cho container) và **chưa có syscall `cont_destroy`**. Khi tạo đủ 7 container, lệnh tiếp theo sẽ báo lỗi an toàn:
   ```text
   container-run: cannot create container: no free slots available or not host
   ```
   Để reset lại 7 slot, cần reboot lại máy ảo QEMU.
2. **Kế thừa container (`-i`):** Khi tiến trình đầu tiên trong container (PID 1) kết thúc, PID namespace của container đó sẽ tự động đóng lại theo thiết kế bảo mật. Lúc này, gọi `container-run -i <id>` sẽ bị kernel từ chối `cont_enter`. Cờ `-i` chỉ áp dụng khi container đang có tiến trình nền hoạt động.
3. **Hiện tượng QEMU không nhận phím (DEFECT-01):** Nếu gặp hiện tượng console không nhận ký tự, hãy khởi động lại QEMU bằng lệnh `make clean && make fs.img && make qemu`.
