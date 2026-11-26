# guestKernel

A small experimental guest kernel, written in C++, that runs directly on Linux KVM. It is built for learning purposes only and is not intended for production or real hardware use.

## Overview

This project pairs a minimal host-side VMM with a guest kernel, both in C++. The host uses the Linux KVM API (`/dev/kvm`) to create a VM and vCPU and to run the guest. QEMU is not involved.

## Boot flow

1. **Starts in 32-bit protected mode.** Real mode is skipped: the host configures the vCPU through KVM so the guest begins already in protected mode.
2. **Transitions to long mode.** The guest switches from protected mode to 64-bit long mode itself.
3. **Runs in the higher half.** After the transition, the kernel executes from a higher-half virtual address.
4. **Emulates I/O devices.** The host handles the guest's port I/O and memory-mapped I/O (MMIO) and emulates the devices the guest needs.

## Requirements

- x86-64 Linux host with KVM enabled (`/dev/kvm` accessible)
- g++ (GCC) by default; Clang also works after editing the compiler settings in the Makefile
- Make

## Build and run

```sh
$ make
$ make run
```

## Status and scope

This is an experiment, so expect missing features and rough edges. It is a learning project, with no stability, security, or compatibility guarantees. Parts of the design are intentionally not documented here.
## Code style

Indentation is one tab per level, chosen for clarity and compactness. If your editor renders tabs wider than you'd like, set the tab width to 4 (or whatever you prefer) rather than converting to spaces.
