# Phase 2 Implementation Summary

Here is a detailed breakdown of how each challenge from Phase 2 was solved.

## 0. Prerequisite: Keyboard
- Implemented `keyboard_getchar` by laying out a basic US QWERTY lookup table corresponding to PS/2 Set 1 Make scancodes.
- Integrated a live typing buffer into the `kmain` render loop, proving that input interrupts and visual text updates are fully operational.

## 1. Physical Memory & Block-Based Arena (`kernel/memory.c`)
- Sourced the memory map from the Limine boot protocol to discover usable contiguous chunks of physical memory.
- Established a basic bitmap-based physical memory manager (PMM), allocating 1 bit per 4KB page. We dynamically mark regions as free based on `LIMINE_MEMMAP_USABLE`.
- Built the application-level arena allocator on top of the PMM. `app_arena_init` requests an initial chain of 64KB physical blocks (using 16 contiguous pages each). `app_malloc` then seamlessly divvies out space from these blocks and seamlessly appends new blocks to the linked list if a large allocation overflows the active block.

## 2. AHCI Storage Driver & PCI Enumeration (`drivers/storage/ahci.c`)
- Implemented a brute-force PCI enumerator over port `0xCF8`/`0xCFC` to locate the first device matching the Class Code for Mass Storage / SATA / AHCI.
- Safely mapped the Physical Memory Mapped I/O (MMIO) registers into the kernel's virtual address space by applying Limine's Higher Half Direct Map (`g_hhdm_offset`) to the `bar5` address.
- Created `virt_to_phys` to translate the statically allocated `.bss` variables for the Command List Base (`clb`), FIS base (`fb`), and Command Tables into hardware-safe physical pointers for DMA.
- Set up LBA48 read/write command primitives (`ahci_do_cmd`) that construct proper PRDT entries to pipe data from disk directly to our kernel buffers.

## 3. FAT32 Filesystem (`fs/fat32.c`)
- Engineered a lightweight FAT32 BPB (BIOS Parameter Block) parser that scans LBA 0 (or hops the MBR) to locate the root cluster, FAT size, and data offset.
- Added `fs_read_file` to perform a naive filename search against standard 8.3 FAT32 directory entries in the root cluster. Once found, it translates the cluster number into an LBA address and calls into the AHCI driver to pull the sectors into memory.

## 4. QEMU Automation (`run_qemu.sh`)
- Overhauled the test script to construct a virtual `disk.img` on-the-fly using `dd` and `mkfs.fat`.
- Used `mcopy` (from `mtools`) to inject a dummy `test.txt` into the virtual FAT32 image.
- Booted QEMU with AHCI attachments (`-device ahci` and `-device ide-hd`) alongside the UEFI firmware to simulate a real hardware setup. The kernel then successfully mounts the drive, finds `test.txt`, and draws the text string onto the screen.
