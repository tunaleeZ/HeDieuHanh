# SCRUM-64.3 - CPU Quota Design

## 1. Muc tieu

Chot thiet ke co che gioi han CPU theo container trong xv6-riscv.
Cac truong du lieu va API user da co khung tu SCRUM-89; accounting,
period reset va scheduler enforcement chua duoc trien khai.

## 2. Trang thai kernel hien tai

- QEMU su dung 3 hart.
- Moi hart khoi tao timer trong kernel/start.c.
- Timer duoc xu ly boi devintr() va clockintr().
- Chi hart 0 tang bien ticks toan cuc.
- usertrap() va kerneltrap() goi yield() khi co timer interrupt.
- scheduler() chon tien trinh RUNNABLE.
- struct container da co cac truong cpu_period_ticks, cpu_quota_ticks,
  cpu_used_ticks va cpu_period_start; logic accounting va enforcement
  chua duoc trien khai.

## 3. Muc tieu accounting

Phan biet:
- Wall-clock ticks: ticks toan cuc, dung cho chu ky quota.
- CPU-ticks: thoi gian CPU tieu thu tren mot hart.

CPU usage cua container la tong CPU-ticks cua cac tien
trinh thuoc container tren tat ca hart. Quota la soft quota.

Khong duoc chi cong ticks toan cuc de suy ra CPU usage.
Moi timer interrupt khi process dang chay duoc tinh la mot CPU-tick
xap xi; do phan giai timer khong the tao gioi han strict.

## 4. Cau truc de xuat

struct container co cac truong:

    uint64 cpu_period_ticks;
    uint64 cpu_quota_ticks;
    uint64 cpu_used_ticks;
    uint64 cpu_period_start;

Gia tri mac dinh va don vi:
- cpu_period_ticks = 20 wall-clock ticks.
- cpu_quota_ticks = 0, nghia la khong gioi han.
- cpu_used_ticks = 0 khi bat dau/reset period.
- cpu_period_start la moc wall-clock ticks cua period hien tai; khi
  tao container, dat bang ticks hien tai.
- Quota la so CPU-tick trong moi period; vi du quota 5, period 20
  tuong duong ngan sach trung binh 25% cua mot CPU, khong phai 25%
  tong 3 CPU.

## 5. Thuat toan de xuat

### 5.1. Period reset

Su dung ticks toan cuc de xac dinh thoi diem bat dau
chu ky moi.

Khi scheduler xem xet mot process RUNNABLE, goi
container_cpu_allowed(container). Duoi container->lock:
- Neu cpu_quota_ticks == 0 thi cho phep.
- Neu ticks - cpu_period_start >= cpu_period_ticks, tinh so period
  da troi qua; cong moc bat dau them so period do nhan period va dat
  cpu_used_ticks ve 0.
- Cho phep neu cpu_used_ticks < cpu_quota_ticks; nguoc lai bo qua
  process trong scheduler cho den khi period duoc reset.

Can xu ly truong hop nhieu chu ky da troi qua trong mot lan kiem tra.

### 5.2. CPU accounting

Khi timer interrupt xay ra tren hart dang chay process, usertrap()
va kerneltrap() tai nhanh which_dev == 2 kiem tra process/container
khong null va cong cpu_used_ticks cua container duoi container->lock.
Thuc hien charge truoc yield(). Moi interrupt tren moi hart duoc
tinh mot CPU-tick.

Cach tinh 1 interrupt = 1 CPU-tick chi la xap xi va tinh ca thoi gian
process dang chay trong kernel mode.

### 5.3. Scheduler

Truoc khi chuyen process RUNNABLE sang RUNNING, scheduler kiem tra
container_cpu_allowed(). Neu het quota, nha p->lock va bo qua process;
process van giu trang thai RUNNABLE. Container 0 la host va duoc mien
throttle; scheduler luon cho phep process cua host.

Khong giu container.lock qua swtch().

## 6. Dong bo nhieu hart

Quy uoc lock: wait_lock -> p->lock -> container->lock khi can giu
nhieu lock cung luc. Scheduler doc quota theo thu tu p->lock roi
container->lock. cont_enter() reserve quota container dich truoc khi
lay p->lock, giu p->lock chi quanh phep gan p->container, roi release
quota container cu sau khi nha p->lock.

Khong giu container->lock qua swtch(), yield(), begin_op(), iput()
hoac kalloc().

Khong duoc goi yield() trong khi dang giu them
container.lock.

### 6.1. Gia dinh va rui ro can kiem chung

- Do phan giai tick: mot timer interrupt duoc xem la mot CPU-tick
  xap xi; can kiem chung sai so thuc te tren QEMU 3 hart.
- container_cpu_allowed() doc ticks ma khong giu tickslock trong luc
  dang giu container->lock. Day la doc xap xi; can xac nhan an toan va
  chap nhan duoc trong implementation.
- Timer drift va viec chi hart 0 tang ticks co the lam moc wall-clock
  khong trung hoan toan voi thoi gian chay tren cac hart; can do sai lech.
- Process co the bi preempt va throttle khi dang giu sleeplock (vi du
  lock inode). Process khac can lock do co the phai cho den khi quota
  reset; can test deadlock/starvation va thoi gian cho, khong coi day la
  van de da duoc giai quyet.

## 7. Tranh starvation va busy-loop

Tien trinh het quota van giu trang thai RUNNABLE,
nhung scheduler tam thoi khong chon.

Can dam bao timer interrupt tiep tuc danh thuc hart
va quota reset de process duoc lap lich lai.

Can kiem tra truong hop khong con process duoc phep
chay va scheduler su dung wfi.

## 8. API chot

API chot:
- int cont_set_cpu(int id, int quota, int period), syscall 30.
- Chi host process duoc cau hinh container; id phai la container active
  va khac 0. quota >= 0, period > 0, quota <= period.
- quota == 0 nghia la khong gioi han; period mac dinh la 20 ticks.
- cont_info() cung cap cpu_period_ticks, cpu_quota_ticks va
  cpu_used_ticks; khong co syscall cont_cpu_info() rieng.

Yeu cau quyen host-only va ma loi -1 duoc hien thuc trong task
permission rieng. Host container id 0 duoc mien throttle.

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

Thiet ke CPU quota da duoc chot; accounting va scheduler enforcement
van chua duoc trien khai. Cac gia dinh va rui ro trong muc 6.1 can duoc
ghi nhan khi chay cac ca CPU-01..CPU-09.
