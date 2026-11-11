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

## 7. Cac van de da duoc nhom quyet dinh

### 7.1. CPU quota la soft quota hay strict quota?

**Quyet dinh:** CPU quota la soft quota.

**Ly do:** Accounting dua tren timer interrupt co do phan giai huu han. Tren nhieu hart, process co the tiep tuc chay trong khoang giua luc cham quota va lan scheduler kiem tra tiep theo.

**He qua:** Container co the vuot quota xap xi toi da mot CPU-tick tren moi hart dang chay; quota khong phai gioi han thoi gian tuyet doi.

### 7.2. CPU usage tinh theo interrupt hay thoi gian chay thuc?

**Quyet dinh:** Moi timer interrupt den khi process dang chay duoc tinh la mot CPU-tick xap xi. Accounting thuc hien trong usertrap() va kerneltrap() tai nhanh which_dev == 2, truoc yield().

**Ly do:** xv6 da preempt process tai nhanh timer interrupt nay; cach dem co the gan voi ca thoi gian chay user va kernel ma khong can them dong ho moi.

**He qua:** So lieu la xap xi, phu thuoc do phan giai va drift cua timer. CPU-tick tren cac hart duoc cong don theo container.

### 7.3. Lock ordering giua process va container?

**Quyet dinh:** Thu tu lock tong the la wait_lock -> p->lock -> container->lock.

**Ly do:** wait_lock da duoc quy uoc nam truoc p->lock trong quan ly parent/process; scheduler can doc container khi dang giu p->lock, nen container->lock phai dung sau p->lock neu hai lock cung duoc giu.

**He qua:** Chi giu p->lock quanh phep gan p->container. Khong giu container->lock khi goi swtch(), yield(), begin_op(), iput() hoac kalloc(). Cac thao tac reserve/release quota khong duoc tao thu tu long nhau nguoc voi quy uoc tren.

### 7.4. CPU quota co ap dung cho host container khong?

**Quyet dinh:** Host container (id 0) duoc mien CPU quota.

**Ly do:** Host chua cac tien trinh quan tri va shell can tiep tuc phan hoi khi container khac bi throttle.

**He qua:** Scheduler khong throttle tien trinh thuoc host theo CPU quota; quota chi ap dung cho container khong phai host.

### 7.5. Container root la per-process hay per-container?

**Quyet dinh:** Root filesystem la thuoc tinh cua container, duoc luu tren struct container va chi duoc dat mot lan bang cont_setroot(id, path), syscall 29. Khi cont_enter thanh cong, tien trinh nhan root va cwd tu root cua container neu root da duoc dat.

**Ly do:** Cac tien trinh cung container can chia se cung mot bien gioi filesystem.

**He qua:** p->root van duoc giu de namex() su dung theo process, nhung gia tri cua no theo root container khi vao container. chroot() duoc giu lai va chi danh cho host theo quyet dinh quyen.

### 7.6. Pham vi memory quota: user pages hay ca kernel pages?

**Quyet dinh:** Chi dem cac trang vat ly user-space da duoc cap phat thuc su; khong dem page table, trapframe hay kernel pages.

**Ly do:** sbrk mac dinh lazy, nen p->sz co the tang ma chua cap phat trang vat ly.

**He qua:** Tang/giam quota theo cac hook cap phat va thu hoi trang user thuc te; khong suy ra memory usage tu p->sz.

### 7.7. Khi nao trien khai PID namespace va container-run?

**Quyet dinh:** Tuan 3 chi khao sat va thiet ke PID namespace. Trien khai PID namespace va container-run tool de sang tuan sau.

**Ly do:** Can tach rieng phan thiet ke va phan hien thuc de danh gia anh huong den quan ly PID, wait/kill va cong cu chay container.

**He qua:** Tuan 3 khong dua PID namespace vao acceptance criteria cua kernel; container-run tool cung nam ngoai pham vi tuan 3.

### 7.8. So syscall lien quan

| So | Syscall | Trang thai |
|---:|---|---|
| 28 | sysinfo | Da su dung |
| 29 | cont_setroot | Da chot so |
| 30 | cont_set_cpu | Da chot so |
| 31 | cont_set_mem | Da chot so |

### 7.9. Quy uoc lock

Khi can giu dong thoi nhieu lock, thu tu la wait_lock -> p->lock -> container->lock. Khong giu container->lock qua swtch(), yield(), begin_op(), iput() hoac kalloc(). Phep gan p->container phai nam trong p->lock; reserve quota container dich thuc hien truoc khi lay p->lock va release quota container cu sau khi nha p->lock.

## 8. Ket luan

SCRUM-61, SCRUM-62 va SCRUM-63 da co cac thanh phan
hoat dong va ket qua kiem thu co ban.

CPU quota, memory quota, PID namespace va cong cu
container-run can duoc tiep tuc trien khai, review va test.

Tai lieu nay khong khang dinh cac tinh nang chua co
bang chung da hoan thanh.

## 9. Week 3 Architecture Status Update (2026-11-11)

This section supersedes outdated implementation-status
statements in sections 2-8, while preserving the original review.

### 9.1 Implemented components

- Container process membership and process quota.
- Container root stored in struct container.
- cont_setroot syscall (29).
- Host-only administration permissions.
- cont_set_cpu syscall (30).
- Timer-based CPU accounting and scheduler throttling.
- CPU quota period reset.
- cont_set_mem syscall (31).
- Memory quota configuration and reserve/release helpers.
- Extended container_info fields.

### 9.2 Verified tests

- Clean build and xv6 boot: PASS.
- forktest: PASS.
- ctest: PASS.
- ptest_neg: PASS.
- roottest with configured root: PASS.
- chroottest: PASS.
- cputest: PASS in documented runs.
- usertests -q: ALL TESTS PASSED.
- Resource benchmarks: results recorded.

### 9.3 Critical open issues

F1 - BLOCKER:
Entering a container without a configured root can
retain the previous host root/cwd.
Configured-root tests do not cover this case.

F2 - MAJOR:
Timer-only CPU charging misses execution that ends
before the next timer interrupt.

F3 - MAJOR:
Throttling while holding a sleeplock may delay
other processes. Dedicated test not yet completed.

F4 - MINOR:
Memory reserve integer addition may overflow.

### 9.4 Pending features

- PID namespace: design only (SCRUM-90).
- Container run tool: not verified/implemented.
- Memory quota VM hooks: not connected.
- Container destroy/reuse lifecycle: not implemented.
- Full concurrent-container isolation tests: pending.

### 9.5 Integration result

SCRUM-88 documents test cases T01-T16 with evidence.
Some cases PASS, others remain BLOCKED or NOT RUN.

See:
- docs/Review_week3_SCRUM82_87.md
- docs/integration_test_matrix.md
- docs/regression_defects.md
- docs/pid_namespace_design.md

The system is not yet considered fully isolated because
the root-unset entry case remains open.
