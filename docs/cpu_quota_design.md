# SCRUM-64.3 - CPU Quota Design

## 1. Muc tieu

Thiet ke co che gioi han CPU theo container trong xv6-riscv.
Tai lieu nay la ban thiet ke, chua trien khai CPU quota vao kernel.

## 2. Trang thai kernel hien tai

- QEMU su dung 3 hart.
- Moi hart khoi tao timer trong kernel/start.c.
- Timer duoc xu ly boi devintr() va clockintr().
- Chi hart 0 tang bien ticks toan cuc.
- usertrap() va kerneltrap() goi yield() khi co timer interrupt.
- scheduler() chon tien trinh RUNNABLE.
- struct container chua co CPU accounting va CPU quota.

## 3. Muc tieu accounting

Phan biet:
- Wall-clock ticks: ticks toan cuc, dung cho chu ky quota.
- CPU-ticks: thoi gian CPU tieu thu tren mot hart.

CPU usage cua container la tong CPU-ticks cua cac tien
trinh thuoc container tren tat ca hart.

Khong duoc chi cong ticks toan cuc de suy ra CPU usage.

## 4. Cau truc de xuat

Du kien bo sung vao struct container:

    uint64 cpu_period_ticks;
    uint64 cpu_quota_ticks;
    uint64 cpu_used_ticks;
    uint64 cpu_period_start;

Thong so khoi dau de thu nghiem:
- Period = 20 wall-clock ticks
- Quota = 5 CPU-ticks
- QEMU = 3 hart

Quota 5/20 tuong duong ngan sach trung binh 25%
cua mot CPU, khong phai 25% tong 3 CPU.

## 5. Thuat toan de xuat

### 5.1. Period reset

Su dung ticks toan cuc de xac dinh thoi diem bat dau
chu ky moi.

Khi qua chu ky:
- Cap nhat moc chu ky.
- Dat lai cpu_used_ticks.
- Cho phep container tiep tuc duoc scheduler xem xet.

Can xu ly truong hop nhieu chu ky da troi qua.

### 5.2. CPU accounting

Khi timer interrupt xay ra tren hart dang chay process:
- Xac dinh container cua process.
- Ghi nhan CPU usage vao container.
- Bao ve bo dem bang spinlock.

Cach tinh 1 interrupt = 1 CPU-tick chi la xap xi.
Can danh gia do chinh xac, timer drift va thoi gian
kernel mode.

### 5.3. Scheduler

Truoc khi chuyen process RUNNABLE sang RUNNING:
- Kiem tra container con ngan sach hay khong.
- Neu het quota, bo qua process trong chu ky hien tai.
- Neu con, cho phep scheduling.

Khong giu container.lock qua swtch().

## 6. Dong bo nhieu hart

Can review:
- Race condition giua scheduler va timer.
- Thu tu khoa p->lock va container.lock.
- Race khi cont_enter() thay doi container cua process.
- Nhieu hart dong thoi chay process cung container.
- Kha nang quota bi vuot do do phan giai timer.
- Co che dat truoc ngan sach de gioi han chat che hon.

Khong duoc goi yield() trong khi dang giu them
container.lock.

## 7. Tranh starvation va busy-loop

Tien trinh het quota van giu trang thai RUNNABLE,
nhung scheduler tam thoi khong chon.

Can dam bao timer interrupt tiep tuc danh thuc hart
va quota reset de process duoc lap lich lai.

Can kiem tra truong hop khong con process duoc phep
chay va scheduler su dung wfi.

## 8. API de xuat

Co the bo sung:
- cont_set_cpu(id, quota, period)
- cont_cpu_info(id, info)

Day la API de xuat, chua dang ky syscall.
Can thong nhat quyen cau hinh, gia tri 0, ma loi
va cach truy cap tu user space.

## 9. Ke hoach kiem thu

CPU-01: Container khong co quota van chay binh thuong.
CPU-02: Mot tien trinh CPU-bound bi gioi han.
CPU-03: Hai tien trinh cung container chia se quota.
CPU-04: Hai container co quota khac nhau.
CPU-05: Process duoc chay lai sau period reset.
CPU-06: Hoat dong dong thoi tren 3 hart.
CPU-07: Host shell van phan hoi khi container het quota.
CPU-08: fork/exit/wait va process quota khong regression.
CPU-09: chroot/path boundary khong regression.

## 10. Ket luan

CPU quota chua duoc trien khai.
Thiet ke can duoc nhom review truoc khi thay doi scheduler,
timer va cau truc container.
