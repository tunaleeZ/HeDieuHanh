# HANDOFF - SCRUM-64 Architecture Review

Project: Lightweight Containers - xv6-riscv
Branch: main
Scope: Architecture review and CPU quota design

## 1. Ket qua

- Da review container/process data structures.
- Da review scheduler va timer interrupt.
- Da phan tich ket qua SCRUM-61, 62, 63.
- Da de xuat thiet ke CPU quota.
- Da xac dinh cac architecture gaps.

## 2. Tai lieu ban giao

- docs/cpu_quota_design.md
- docs/architecture_review_scrum64.md
- docs/architecture_draft_vi.md (Sprint 1 reference)

## 3. Nguon ma da khao sat

- kernel/proc.h
- kernel/proc.c
- kernel/trap.c
- kernel/start.c
- kernel/kalloc.c
- kernel/vm.c

## 4. Quy trinh Git

Nhom lam viec truc tiep tren main.
Khong tao feature branch.
Can fetch/review truoc khi push.
Khong su dung force push.

## 5. Van de chua hoan thanh

- CPU quota chua duoc code.
- Memory quota chua duoc code.
- PID namespace chua duoc xac minh.
- Design review chua duoc nhom phe duyet.
- Chua co regression test CPU quota.

## 6. De xuat buoc tiep theo

1. Nhom review va chot CPU quota policy.
2. Thong nhat syscall API va lock ordering.
3. Lap ke hoach prototype CPU quota neu con thoi gian.
4. Chay integration/regression theo SCRUM-65.
5. Ghi nhan ket qua PASS/FAIL/NOT RUN vao Jira.

Status: Documentation prepared; pending team review.
