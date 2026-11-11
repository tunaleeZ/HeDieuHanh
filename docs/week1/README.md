
# xv6-riscv Development Environment

## 1. Project Information

- Project: Lightweight Containers on xv6-riscv
- Repository directory: `~/xv6_lab/xv6-riscv`
- Host username: `tuan`
- Hostname: `NguyenAnhTuan-20233698`

## 2. Development Environment

| Component | Version / Information |
|---|---|
| Host operating system | Ubuntu |
| Linux kernel | 7.2.5 |
| Host architecture | x86_64 |
| GCC (default host compiler) | 15.2.0 |
| QEMU | 10.2.1 |
| Target operating system | xv6-riscv |
| Virtualization / emulation | QEMU |

## 3. Environment Verification Commands

Run the following commands on the Ubuntu host to inspect the environment:

```bash
gcc --version
qemu-system-riscv64 --version
uname -a
```

## 4. Build and Run xv6-riscv

From the repository root:

```bash
cd ~/xv6_lab/xv6-riscv
make qemu
```

To rebuild from a clean state:

```bash
make clean
make qemu
```

## 5. Basic User-Space Verification

After xv6 boots, run these commands at the xv6 shell prompt:

```text
ls
echo hello
```

Expected results:
- xv6-riscv boots successfully in QEMU.
- The xv6 shell prompt appears.
- The `ls` and `echo` commands execute successfully.

To exit QEMU using the default xv6 Makefile configuration, press `Ctrl+A`, then `X`.

## 6. Build Verification Status

- [ ] xv6-riscv builds successfully.
- [ ] xv6-riscv boots successfully in QEMU.
- [ ] `ls` runs successfully in the xv6 shell.
- [ ] `echo hello` runs successfully in the xv6 shell.
- [ ] xv6-riscv can be rebuilt after a source change.

## 7. Notes

The GCC version above is the default compiler installed on the Ubuntu host. The actual compiler used to build xv6-riscv may be a RISC-V cross-compiler selected by the project's Makefile.

Record the compiler configuration and any build issues here after verification.
