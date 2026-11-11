# Scrum85 – CPU Quota Prototype – Handoff
Cập nhật lần cuối: 2026-11-11  | Trạng thái task: hoàn tất

## 1. Mục tiêu và Acceptance Criteria
Triển khai CPU quota soft theo `docs/cpu_quota_design.md`: cấu hình quota, đếm timer tick trên từng hart, reset period và bỏ qua tiến trình đã dùng hết quota. Chạy `cputest` ít nhất ba lần; xác nhận host vẫn có tiến độ và mỗi cửa sổ quota được làm mới.

## 2. Trạng thái sub-task
| Sub-task | Trạng thái | Commit hash | Ghi chú |
| --- | --- | --- | --- |
| 4.1 Initialize CPU fields and implement cont_set_cpu syscall (30) | đã commit | 92ea44c | Fields khởi tạo; setter host-only active non-host, kiểm quota/period |
| 4.2 Charge CPU ticks on timer interrupt | đã commit | 8458cf3 | Mỗi timer interrupt của process cộng một CPU tick dưới container lock |
| 4.3 Implement period reset helper | đã commit | 741a259 | Quota 0/host luôn allowed; elapsed periods reset budget |
| 4.4 Skip throttled processes in scheduler | đã commit | d916037 | Runnable bị throttle được skip; giữ nguyên wfi khi không process nào chạy được |
| 4.5 Write cputest, run and save evidence | đã commit | cc2a6e3 | Ba lượt 200 tick PASS; worker quota có tiến độ mọi cửa sổ, host có progress |

## 3. Quy ước và giả định
- Host container 0 được miễn throttle. `quota == 0` là không giới hạn; đơn vị quota là CPU tick xấp xỉ, period tính theo wall-clock `ticks`.
- Thứ tự lock khi scheduler xét tiến trình: `p->lock` rồi `container->lock`; không giữ container lock qua `swtch()`.
- Đọc `ticks` không giữ `tickslock` là xấp xỉ, như thiết kế D4. Timer interrupt trên mỗi hart được tính một CPU tick.
- Đây là soft quota: một interrupt xấp xỉ một CPU tick; số vòng lặp không phải phép đo chu kỳ CPU đã hiệu chuẩn. Sai số độ phân giải và timer drift chưa được định lượng.
- Không quan sát thấy deadlock/busy-loop trong ba lượt. Riêng trường hợp tiến trình bị throttle khi đang giữ inode sleeplock chưa được kiểm tra; lock wait kéo dài vẫn là rủi ro cần kiểm thử.

## 4. Bằng chứng kiểm thử
`make` và `make fs.img` PASS (linker cảnh báo RWX LOAD segment như trước). QEMU 10.2.1, 128M, 3 hart: ba lượt `cputest` PASS; mỗi lượt container A dùng quota 5/20 trong 200 tick và làm ít vòng hơn worker host B; mỗi trong 10 cửa sổ đều có tiến độ; parent host tiếp tục in `HOST_PROGRESS`. Transcript: `docs/test_evidence/T10_cpu_quota.txt`, `docs/test_evidence/T11_cpu_refresh.txt`.

## 5. Quy ước lock và idle
Scheduler giữ `p->lock` rồi gọi helper giữ `container->lock`; helper nhả container lock trước khi scheduler đổi trạng thái hoặc `swtch()`. Khi mọi process runnable bị throttle, `found` vẫn bằng 0 và scheduler đi vào `wfi`; timer interrupt đánh thức hart để scheduler thử lại. Thứ tự này không tạo vòng lock ngược vì helper không lấy `p->lock`, còn setter chỉ lấy container lock.

## 6. Bước tiếp theo
SCRUM-85 hoàn tất với năm commit. Không push.
