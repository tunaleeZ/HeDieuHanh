# SCRUM-87 — Benchmark rerun tuần 3

## Thiết lập

- QEMU 10.2.1, `virt`, RAM 128 MiB, `-smp 3`.
- Build sạch một lần: `make clean`, `make`, `make fs.img` đều PASS.
- Năm lượt được chạy trong năm lần boot QEMU riêng; mỗi lượt chạy lần lượt `bench_cpu`, `bench_mem`, `bench_proc` từ trạng thái kernel mới.
- Tham số cố định: `bench_cpu` dùng `duration=100` ticks theo `user/bench_cpu.c`; `bench_mem` cấp 32 trang = 131072 bytes; `bench_proc` chạy 100 lần fork/wait.
- Raw output: [run 1](benchmark_run_1.txt), [run 2](benchmark_run_2.txt), [run 3](benchmark_run_3.txt), [run 4](benchmark_run_4.txt), [run 5](benchmark_run_5.txt).

Độ lệch chuẩn dưới đây là population standard deviation (`ddof=0`).

## Kết quả từng lượt

| Run | CPU iterations / 100 ticks | CPU iterations/tick (đầu ra) | Memory used / recovered (bytes) | Fork success / attempts | Elapsed ticks | Process slots trước/sau | RAM trước/sau process (bytes) |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 618954 | 6189 | 131072 / 131072 | 100 / 100 | 0 | 3 / 3 | 133255168 / 133255168 |
| 2 | 617294 | 6172 | 131072 / 131072 | 100 / 100 | 0 | 3 / 3 | 133255168 / 133255168 |
| 3 | 598448 | 5984 | 131072 / 131072 | 100 / 100 | 2 | 3 / 3 | 133255168 / 133255168 |
| 4 | 585704 | 5857 | 131072 / 131072 | 100 / 100 | 1 | 3 / 3 | 133255168 / 133255168 |
| 5 | 589970 | 5899 | 131072 / 131072 | 100 / 100 | 0 | 3 / 3 | 133255168 / 133255168 |

## Thống kê

| Chỉ số | Trung bình | Độ lệch chuẩn | Ghi chú |
|---|---:|---:|---|
| CPU iterations / 100 ticks | 602074 | 13742.01 | Tổng số vòng lặp |
| CPU iterations/tick | 6020.2 | 137.24 | Trung bình các giá trị đã in, mỗi giá trị bị làm tròn xuống |
| Memory used | 131072 bytes | 0 | 32 trang được chạm thực tế |
| Memory recovered | 131072 bytes | 0 | RAM quay lại mức trước cấp phát |
| Fork thành công | 100 / 100 | 0 | Mọi lượt PASS |
| Process elapsed ticks | 0.6 | 0.8 | Timer tick thô; phạm vi quan sát 0–2 |
| Process slots trước/sau | 3 / 3 | 0 | Không rò slot |
| Free RAM trước/sau process | 133255168 / 133255168 bytes | 0 | Không đổi giữa các lượt |

## So sánh baseline và chênh lệch CPU

`architecture_review_scrum64.md` ghi baseline CPU là 230409 iterations trong 100 ticks, tương đương 2304.09 iterations/tick. Trung bình tuần này là 602074 iterations/100 ticks, hay 6020.2 iterations/tick theo trung bình các dòng đã làm tròn. Memory khớp baseline 131072 bytes cấp phát/thu hồi; process khớp 100/100 fork và 3 slot trước/sau. Baseline process ghi 2 ticks; lần chạy mới 0–2 ticks (trung bình 0.6), nằm trong sai số đo thời gian ngắn bằng tick thô.

`HANDOFF_SCRUM65_Integration_and_Regression.md` ghi `bench_cpu` là 6539 iterations/tick, nhưng không lưu tổng iterations, duration thực chạy hay raw transcript của lần đó. Mã benchmark được lưu từ SCRUM-63 và đang dùng ở đây đặt `duration=100`; handoff SCRUM-65 cũng chỉ ghi cấu hình QEMU `-smp 3`, không ghi một tham số CPU benchmark khác. Vì vậy phép chuẩn hóa cho baseline đầu cho ra 2304.09/tick, còn baseline thứ hai là 6539/tick; các tham số được lưu không đủ để giải thích nguyên nhân chênh lệch 2.84x và không chứng minh hai số được đo trong cùng điều kiện. Không gán nguyên nhân giả định cho tham số bị thiếu. Lần rerun này khóa `duration=100` và ghi cả tổng lẫn tốc độ để các lần tiếp theo so sánh được.

## Kết luận

Năm lần chạy đều hoàn tất. Memory accounting và fork/wait giữ đúng invariant trong cả năm lượt. CPU có độ biến thiên khoảng 2.3% theo population SD; cần dùng các lượt cố định mới làm baseline lặp lại, thay vì suy diễn nguyên nhân cho số lịch sử thiếu tham số.
