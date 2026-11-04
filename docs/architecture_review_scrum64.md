# SCRUM-64 - Container Architecture Review

## 1. Pham vi

Rà soat kien truc hien tai cua Lightweight Containers
tren xv6-riscv, dua tren SCRUM-61, SCRUM-62 va SCRUM-63.

## 2. Process management - SCRUM-61

Da trien khai:
- Bang container co dinh, NCONTAINER = 8.
- Container 0 la host container.
- struct proc chua con tro struct container *.
- cont_create(), cont_enter(), cont_info().
- Process quota kiem tra trong kfork().
- Thu hoi quota trong freeproc().

Kiem thu:
- ctest PASS voi cac kich ban co ban.
- Chua co bang chung stress/concurrency day du.

Gioi han:
- Chua co destroy container.
- Chua co PID namespace doc lap.
- Chua co permission model cho container syscall.

## 3. Filesystem isolation - SCRUM-62

Da trien khai:
- struct proc chua root inode.
- fork ke thua root.
- namex() su dung process root cho absolute path.
- Xu ly .. tai root boundary.
- Syscall chroot().

Kiem thu:
- chroottest PASS.
- roottest PASS voi rootfs da chuan bi.

Luu y kien truc:
- Root duoc luu theo process, khong phai container.
- Chua chung minh tat ca process cung container
  bat buoc su dung cung mot root.
- Can review file descriptor da mo truoc chroot.

## 4. Resource measurement - SCRUM-63

Da trien khai:
- sysinfo() syscall so 28.
- kfreemem(): dem free physical pages.
- knproc(): dem used process slots.
- bench_cpu, bench_mem, bench_proc.

Baseline:
- CPU: 230409 iterations trong 100 ticks.
- Memory: cap phat 131072 bytes, thu hoi 131072 bytes.
- Process: 100/100 fork thanh cong trong 2 ticks.
- Process slots: 3 truoc va sau benchmark.
- RAM: 133255168 bytes truoc va sau process benchmark.

Day la ket qua mot lan do, chua phai trung binh.

## 5. CPU scheduler va timer

- scheduler() duyet cac process RUNNABLE.
- Moi hart co timer interrupt.
- Chi hart 0 tang ticks toan cuc.
- Timer interrupt co the yeu cau yield().
- Chua co CPU quota trong scheduler.

Thiet ke chi tiet:
docs/cpu_quota_design.md

## 6. Gap analysis

| Yeu cau | Trang thai |
|---|---|
| Container process membership | Implemented |
| Process quota | Basic tests PASS |
| Root isolation | Basic tests PASS |
| System resource monitoring | Basic tests PASS |
| CPU benchmark | Baseline available |
| Memory benchmark | Baseline available |
| Process benchmark | Baseline available |
| CPU quota | Design only |
| Memory quota per container | Not implemented |
| PID namespace PID 1 | Not verified |
| Container run tool | Not verified |
| Isolation stress tests | Not completed |

## 7. Cac van de can nhom quyet dinh

1. CPU quota la soft quota hay strict quota?
2. CPU usage tinh theo interrupt hay thoi gian chay thuc?
3. Lock ordering giua process va container?
4. CPU quota co ap dung cho host container khong?
5. Container root la per-process hay per-container?
6. Pham vi memory quota: user pages hay ca kernel pages?
7. Khi nao trien khai PID namespace va container-run?

## 8. Ket luan

SCRUM-61, SCRUM-62 va SCRUM-63 da co cac thanh phan
hoat dong va ket qua kiem thu co ban.

CPU quota, memory quota, PID namespace va cong cu
container-run can duoc tiep tuc trien khai, review va test.

Tai lieu nay khong khang dinh cac tinh nang chua co
bang chung da hoan thanh.
