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

# Phase 3 Implementation Summary

Here is a detailed breakdown of how each challenge from Phase 3 was solved.

## 1. RSLlinker Manifest Parser (`runtime/rsllinker.c`)
- Sourced the minimal, zero-allocation `jsmn` JSON library to safely parse manifests inside our freestanding environment.
- Configured `JSMN_STATIC` to build the parsing routines directly into our translation unit.
- Implemented `parse_rsllink` to navigate the JSMN token array sequentially. It accurately matches strings against JSON keys (without needing standard `string.h` library dependencies) and populates the `rsl_manifest_t` schema: tracking `name`, window bounds, `entry_entity`, the `entities` array, and the KV pair `assets` mapping.

## 2. BMP Asset Loader (`runtime/bmp_loader.c`)
- Authored a pure-C uncompressed Windows BMP header parser, targeting the standard 54-byte structure (`bmp_file_header_t` and `bmp_info_header_t`).
- Implemented robust geometry bounds checking. Crucially, the loader respects the rigid 4-byte boundary requirement mandated by the Windows BMP format by utilizing the classic row stride algorithm: `stride = ((width * bpp + 31) / 32) * 4`. This ensures horizontally irregular sprites don't suffer from diagonal pixel shearing.
- Expanded `draw_sprite_uv()` to accommodate both 32-bit (utilizing the embedded alpha byte) and 24-bit formats (enacting a strict `#FF00FF` Magenta color key bypass for transparent pixels).

## 3. Host-Side Unit Tests (`tests/`)
- Integrated two test files (`tests/test_rsllinker.c` and `tests/test_bmp.c`) to validate parsing schemas instantaneously.
- Created a `make test` pipeline in the Makefile. By avoiding the freestanding constraints of the kernel (`-ffreestanding -nostdlib`), these tests are compiled against the host system's native `glibc`. This enables continuous integration checks on parser string logic without spinning up full QEMU emulation.

## 4. End-to-End QEMU Validation
- Built a Python script locally to generate a true binary test asset (`test.bmp` - A red square surrounded by a green border).
- Appended `test.rsllink` into the FAT32 virtual `disk.img` generated at test runtime via `mcopy`.
- Rewrote the tail end of `kernel/main.c` to sequentially: boot, mount storage, traverse FAT32, open the manifest, dynamically request memory blocks via `app_malloc`, read the associated BMP asset off the physical disk, and securely map the result to the UEFI double-buffered canvas (`0.5`, `0.5` UV).
