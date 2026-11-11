# Scrum85 – CPU Quota Prototype – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: đang thực hiện

## 1. Mục tiêu và Acceptance Criteria
Triển khai CPU quota soft theo `docs/cpu_quota_design.md`: cấu hình quota, đếm timer tick trên từng hart, reset period và bỏ qua tiến trình đã dùng hết quota. Chạy `cputest` ít nhất ba lần; xác nhận host vẫn có tiến độ và mỗi cửa sổ quota được làm mới.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 4.1 Initialize CPU fields and implement cont_set_cpu syscall (30) | đã commit | 92ea44c | Fields khởi tạo; setter host-only active non-host, kiểm quota/period |
| 4.2 Charge CPU ticks on timer interrupt | đã commit | 8458cf3 | Mỗi timer interrupt của process cộng một CPU tick dưới container lock |
| 4.3 Implement period reset helper | đã commit | sắp ghi | Quota 0/host luôn allowed; elapsed periods reset budget |
| 4.4 Skip throttled processes in scheduler | chưa làm | — | |
| 4.5 Write cputest, run and save evidence | chưa làm | — | |

## 3. Quy ước và giả định
- Host container 0 được miễn throttle. `quota == 0` là không giới hạn; đơn vị quota là CPU tick xấp xỉ, period tính theo wall-clock `ticks`.
- Thứ tự lock khi scheduler xét tiến trình: `p->lock` rồi `container->lock`; không giữ container lock qua `swtch()`.
- Đọc `ticks` không giữ `tickslock` là xấp xỉ, như thiết kế D4. Timer interrupt trên mỗi hart được tính một CPU tick.
- Cần ghi nhận độ phân giải timer, timer drift và rủi ro tiến trình bị throttle khi đang giữ sleeplock.

## 4. Bằng chứng kiểm thử
4.1–4.3: `make` PASS (linker cảnh báo RWX LOAD segment như trước). Các ca runtime sẽ chạy ở 4.5.

## 5. Bước tiếp theo
Hoàn tất 4.1 rồi lần lượt 4.2–4.5; mỗi sub-task có một commit riêng. Không push.
