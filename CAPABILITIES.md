# RSL-OS: Current System Capabilities

RSL-OS is a modern, 64-bit x86_64 bare-metal operating system built from scratch without any POSIX, Linux, or `libc` dependencies.

As of the conclusion of Phase 4, the operating system is a functional game-console-like platform. It possesses a full hardware stack capable of bootstrapping itself, mounting a filesystem, and interactively running Python-like scripts rendered directly to the screen via an integrated bytecode virtual machine.

Here is an overview of what the system can currently do "like a real OS":

## 1. Hardware Initialization & Boot
*   **UEFI Booting (64-bit Long Mode):** RSL-OS leverages the Limine bootloader protocol to successfully transition from the BIOS/UEFI directly into a fully initialized 64-bit environment, skipping the complexities of legacy 16-bit real mode.
*   **Higher Half Kernel:** The kernel safely executes out of the higher half of virtual memory (`0xffffffff80200000`), cleanly separating OS logic from user application memory space.

## 2. Memory Management
*   **Physical Memory Manager (PMM):** The OS parses the firmware's memory map to discover contiguous regions of usable physical RAM, dynamically managing it with a custom 1-bit-per-4KB bitmap page frame allocator.
*   **Application Arena Allocator:** Higher-level logic utilizes an arena allocator (`app_malloc`) that dynamically claims memory in 64KB physical blocks from the PMM. This allows rapid runtime allocations while guaranteeing safe, instant cleanup by dropping the entire arena when an application closes.

## 3. Storage & Filesystem
*   **PCI Bus Enumeration:** The OS probes the standard PCI configuration space to locate and bind itself to the first active AHCI (SATA) host bus adapter.
*   **Bare-Metal AHCI Driver:** The kernel establishes DMA command lists and FIS structures (Memory Mapped I/O), translating virtual memory addresses back to physical addresses to directly instruct the storage hardware to perform sector reads and writes via LBA48.
*   **FAT32 Parsing:** The filesystem driver parses standard FAT32 BIOS Parameter Blocks (BPB). It can navigate the root directory cluster, locate specific filenames, and load files directly off the physical disk into the system memory arena.

## 4. Graphics & Input
*   **Native GOP Framebuffer:** RSL-OS claims the high-resolution native graphical framebuffer provided by the UEFI firmware.
*   **Double-Buffering & Pitch-Safe Swaps:** To prevent tearing, all rendering is done to a massive off-screen RAM canvas. At the end of every frame, the OS copies this canvas to the screen, meticulously calculating the correct byte pitch to support unpredictable hardware padding.
*   **Normalized UV Rendering Pipeline:** Drawing primitives (Rectangles, Text, Sprites) are positioned using a resolution-agnostic shader-style UV coordinate system (where `0.0, 0.0` is the top-left, and `1.0, 1.0` is the bottom-right).
*   **BMP Asset Loading:** The OS natively parses uncompressed 24-bit and 32-bit Windows BMP files. It resolves 4-byte padding bounds automatically, blends 32-bit alpha channels, and ignores a literal Magenta (`#FF00FF`) color-key for 24-bit sprite transparency.
*   **Interrupt-Free Input Polling:** A native PS/2 keyboard driver scans port `0x60`, maintaining a robust 256-key state array tracking edge-triggered key-presses and held-down states.

## 5. RSL Scripting Engine (The "App" Layer)
*   **JSON Manifest Parser:** Using a minimal, zero-allocation C library (`jsmn`), the OS reads application configuration manifests (`.rsllink`), automatically setting up the environment width, resolving dependencies, and mapping virtual asset keys (e.g., `spr_player`) to real file paths.
*   **Custom Indentation-Sensitive Compiler:** A standalone, native lexer and recursive-descent parser tokenize raw RSL text files (which use a Python-style syntax) and compile them directly into stack-based bytecode instructions directly in memory.
*   **Bytecode Virtual Machine (VM):** RSL-OS runs a lightweight stack-based VM that executes the compiled opcodes. It handles object properties (`self.x`, `self.y`), math expressions, conditional branching, built-in system calls (like `draw_self()` or `key_down()`), and hardware alarms.
*   **60 Hz Event Loop:** The core engine binds the VM into an infinite GameMaker-style lifecycle loop. It manages a pool of active entity instances, automatically ticking hardware alarms, checking for input, running `on step:` logic, rendering the background, executing `on draw:`, and swapping the framebuffer at a stable framerate.
