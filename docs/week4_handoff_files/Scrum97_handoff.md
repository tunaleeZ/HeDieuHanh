# SCRUM-97 – Tích hợp và kiểm thử hồi quy tuần 4

## 1. Mục tiêu

Tích hợp các chức năng do các thành viên phát triển,
kiểm thử toàn bộ hệ thống và đánh giá mức độ
hoàn thành đề tài Lightweight Containers.

## 2. Công việc đã thực hiện

- Tích hợp root isolation.
- Kiểm tra PID namespace.
- Tích hợp memory quota.
- Tích hợp CPU runtime accounting.
- Xây dựng chương trình kiểm thử fork bomb.
- Xây dựng chương trình kiểm thử nhiều container.
- Thực hiện root isolation regression.
- Thực hiện clean build và final regression.
- Phát hiện và khắc phục lỗi hết block filesystem.
- Cập nhật tài liệu kỹ thuật.

## 3. Các chương trình kiểm thử bổ sung

### forkbombtest.c

Kiểm tra khả năng giới hạn tiến trình
khi xảy ra fork stress có giới hạn.

Kết quả:

- 7 fork thành công.
- 9 fork bị từ chối.
- Peak process count: 8.
- Tài nguyên được thu hồi.
- Host duy trì hoạt động.
- 0 failures.

### multi_ctest.c

Kiểm tra đồng thời hai container có:

- Root directory khác nhau.
- PID namespace riêng.
- Process quota khác nhau.
- Memory quota khác nhau.
- CPU quota khác nhau.

Kết quả: 0 failures.

## 4. Kiểm thử hồi quy

- forktest: PASS.
- ctest: 0 failures.
- memquotatest: 0 failures.
- pidns_test: 0 failures.
- cputest: PASS.
- cpuisotest: 0 failures.
- roottest: 0 failures.
- ptest_neg: 0 failures.
- test_sysinfo: PASS.
- usertests -q: ALL TESTS PASSED.

## 5. Khắc phục lỗi filesystem

Trong quá trình kiểm thử, writebig thất bại
do filesystem hết block trống.

Đã tăng FSSIZE từ 2000 lên 4000 blocks.

Sau khi clean build và kiểm thử lại:

    ALL TESTS PASSED
    HOST_OK

## 6. Kết quả tổng hợp

Tổng số test: 16.

- PASS: 14.
- PARTIAL PASS: 1.
- BLOCKED: 1.
- Tỷ lệ PASS: 87,5%.

T12 vẫn còn hạn chế soft CPU quota và locking.

T15 chưa được triển khai.

## 7. Commit liên quan

- 5ebcbea: Kiểm thử fork bomb T06.
- 8b9b7c9: Kiểm thử nhiều container T14.
- 0b0a7e6: Kiểm thử root isolation T13.
- 8b63171: Cập nhật kiểm thử T14.
- 4de9a01: Tăng dung lượng filesystem.

## 8. Bằng chứng kiểm thử

- scrum97_T06_forkbomb.log
- scrum97_T13_root.log
- scrum97_T14_multi.log
- scrum97_final_build.log
- scrum97_final_cpu_pid.log
- scrum97_final_usertests.log
- scrum97_usertests_fsfull.log

Các file nằm trong docs/test_evidence/.

## 9. Công việc tiếp theo

- Cải thiện CPU quota trên nhiều hart.
- Đánh giá và xử lý sleeplock-holder throttling.
- Triển khai destroy/reuse container.
- Xác minh và tích hợp chương trình Container-run.
- Tăng phạm vi kiểm thử fault injection.
- Hoàn thiện sản phẩm và tài liệu cuối kỳ.

## 10. Kết luận

SCRUM-97 đã hoàn thành các hoạt động tích hợp
và kiểm thử chức năng trong phạm vi tuần 4.

Hệ thống đạt phần lớn yêu cầu kỹ thuật và
không ghi nhận lỗi trong lần chạy usertests cuối.

Các hạn chế còn lại đã được ghi nhận rõ ràng.
