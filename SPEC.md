# RSL-OS: 64-Bit Bare-Metal Operating System Specification

## 1. Overview
RSL-OS is an independent, 64-bit x86_64 bare-metal operating system with no Linux or POSIX dependencies.
All system and user applications are written in RSL (Reduced Scripting Language) — an event-driven language combining Pythonic syntax with GameMaker (GML) lifecycle events.

## 2. Kernel & Hardware Layer
- **Bootloader**: Limine Bootloader Protocol (UEFI only, 64-bit Long Mode).
- **Display**: Native high-resolution UEFI GOP Framebuffer. Double-buffered backbuffer in RAM.
- **Coordinates**: Shader-style Normalized UV Coordinates: (0.0, 0.0) is top-left, (1.0, 1.0) is bottom-right. Mapped to the application's requested virtual screen dimensions.
- **Input**: PS/2 controller (ports 0x60/0x64) and UEFI input polling.
- **Storage**: AHCI (SATA) and NVMe block drivers with FAT32 partition reading/writing. Supports live-USB execution and direct disk installation.
- **Memory Management**: Block-based arena allocator. On application launch (`on app_create`), an arena of memory blocks is claimed. Malloc claims additional blocks. On `on app_cleanup`, all allocated blocks are wiped simultaneously.

## 3. The RSLlinker Manifest
Every RSL application is declared via an `.rsllink` manifest defining the asset drawer:
```json
{
  "name": "MyGame",
  "screen_width": 1920,
  "screen_height": 1080,
  "entry_entity": "Player",
  "entities": ["player.rsl", "enemy.rsl"],
  "assets": {
    "spr_player": "assets/player.bmp",
    "spr_enemy": "assets/enemy.bmp"
  }
}
```
## 4. RSL Language & Event Lifecycle
RSL uses Python indentation and syntax with GML-style object-oriented events:

- `on app_create:` / `on app_destroy:` / `on app_cleanup:` (Application lifecycle)
- `on create:` (Entity instance initialization)
- `on step:` (Frame logic; non-drawing)
- `on start draw:` (Pre-draw; clearing / background)
- `on draw:` (Main rendering; defaults to draw_self() if unwritten)
- `on end draw:` (Post-draw; UI / HUD / overlays)
- `on alarm(n):` (Hardware timers where n = 0..15)
- `on destroy:` (Entity gameplay destruction logic)
- `on cleanup:` (Entity memory/handle cleanup)

## 5. Sprites
Uncompressed 24-bit/32-bit Windows BMP files. Entities with assigned sprites automatically render via `draw_self()`.

---

# Part 2: Phased Master Prompts for Jules

Give these tasks to Jules one at a time via GitHub issues:

---

### Issue 1: Kernel Skeleton, UV Framebuffer & Limine Boot
> **Title**: `Phase 1: Bootable 64-bit Kernel with Limine and Normalized UV Framebuffer`
>
> **Task for Jules**:
> ```text
> Read SPEC.md for full context. Implement the bare-metal 64-bit x86_64 kernel core using the Limine boot protocol in freestanding C (-ffreestanding -nostdlib -mno-red-zone).
>
> Deliverables:
> 1. kernel/main.c: Kernel entry point kmain() verifying Limine 64-bit Long Mode handoff.
> 2. kernel/framebuffer.c:
>    - Acquire the native UEFI GOP framebuffer from Limine.
>    - Allocate a double-buffered RAM canvas.
>    - Implement normalized UV coordinate drawing: (0.0, 0.0) is top-left, (1.0, 1.0) is bottom-right.
>    - Implement primitives: clear(color), draw_rect_uv(u1, v1, u2, v2, color), draw_text_uv(str, u, v, color).
>    - Implement swap_buffers() using 64-bit word copies to physical VRAM.
> 3. kernel/keyboard.c: PS/2 keyboard port reader updating key_down and key_pressed states.
> 4. Makefile & limine.cfg: Build a bootable UEFI ISO and provide a run_qemu.sh script testing GOP display and keyboard in QEMU.
> ```

---

### Issue 2: Block-Based Memory Arena & AHCI/NVMe Storage
> **Title**: `Phase 2: Block-Based Memory Allocator and Storage Drivers`
>
> **Task for Jules**:
> ```text
> Read SPEC.md. Implement the memory management and block storage subsystem.
>
> Deliverables:
> 1. kernel/memory.c:
>    - Physical memory manager using the Limine memory map (page frame allocator).
>    - App-level block-based arena allocator:
>      - app_arena_init(initial_blocks): Claims a pool of 64KB blocks when an application starts.
>      - app_malloc(size): Sub-allocates from the current block, or requests another block from the kernel pool.
>      - app_arena_destroy(): Frees all blocks tied to the app at once during app_cleanup.
> 2. drivers/storage/:
>    - Implement an AHCI (SATA) driver using PCI enumeration and Memory-Mapped I/O (ABAR).
>    - Add a stubbed NVMe controller queue interface.
> 3. fs/fat32.c:
>    - Read and write support for FAT32 partitions on AHCI/USB storage.
>    - Functions: fs_read_file(path, out_buf), fs_write_file(path, in_buf), fs_list_dir(path).
> ```

---

### Issue 3: RSLlinker Parser & BMP Asset Drawer
> **Title**: `Phase 3: RSLlinker Manifest Parser and BMP Asset Loader`
>
> **Task for Jules**:
> ```text
> Read SPEC.md. Implement the asset drawer and project linking system.
>
> Deliverables:
> 1. runtime/rsllinker.c:
>    - A lightweight JSON/Key-Value parser that reads `.rsllink` manifest files.
>    - Parses target screen dimensions (e.g. 1920x1080), the entry entity, the list of `.rsl` entity script files, and the dictionary of sprite assets.
> 2. runtime/bmp_loader.c:
>    - Parse uncompressed 24-bit and 32-bit Windows BMP files directly from memory buffers.
>    - Expose sprite_t struct containing pixel arrays, dimensions, and color keys.
>    - Implement draw_sprite_uv(sprite, u, v, scale_x, scale_y) to blit sprites onto the double-buffered screen using normalized coordinates.
> 3. Unit tests verifying manifest parsing and BMP header loading in standalone test targets.
> ```

---

### Issue 4: RSL Compiler, Bytecode VM & GML Event Runner
> **Title**: `Phase 4: RSL Pythonic Compiler, Bytecode VM, and Event Dispatcher`
>
> **Task for Jules**:
> ```text
> Read SPEC.md. Build the RSL language compiler and virtual machine runtime in freestanding C.
>
> Deliverables:
> 1. compiler/lexer.c & compiler/parser.c:
>    - Indentation-sensitive tokenizer (INDENT/DEDENT) with Pythonic syntax.
>    - Parse object declarations: `entity Name:`.
>    - Parse lifecycle events:
>      - `on app_create:`, `on app_destroy:`, `on app_cleanup:`
>      - `on create:`, `on step:`, `on start draw:`, `on draw:`, `on end draw:`, `on alarm(n):`, `on destroy:`, `on cleanup:`.
>    - If an entity does not define `on draw:`, automatically generate a default call to `draw_self()`.
>    - Support both polling (key_down, mouse_x) and event-driven (on key_press) input.
> 2. vm/vm.c:
>    - Fast stack-based bytecode virtual machine.
>    - Supports active entity pools (position, sprite reference, alarm[0..15] array).
> 3. vm/engine.c (The 60 Hz Kernel Loop):
>    - Decrement active alarms; fire `on alarm(n)` when a counter reaches 0.
>    - Run `on step` across all active entities.
>    - Run `on start draw`.
>    - Run `on draw` (or default `draw_self()`) using normalized 0.0 -> 1.0 UV mapping.
>    - Run `on end draw`.
>    - Call swap_buffers().
> ```

---

### Issue 5: Core System Apps (Launcher, Editor & Native Installer)
> **Title**: `Phase 5: Core System Applications in RSL`
>
> **Task for Jules**:
> ```text
> Read SPEC.md. Because all applications are written in RSL, create the default system suite under /system/:
>
> Deliverables:
> 1. system/launcher.rsllink & system/launcher.rsl:
>    - The default desktop shell that launches on boot.
>    - Scans the storage drive for installed `.rsllink` apps, displays them with UV-drawn buttons, and launches the selected app via sys.launch().
> 2. system/editor.rsllink & system/editor.rsl:
>    - A text editor app written in RSL allowing the user to write, edit, and save `.rsl` and `.rsllink` files directly on physical hardware.
> 3. system/installer.rsllink & system/installer.rsl:
>    - The OS installation app: detects internal SATA (AHCI) or NVMe drives, formats an EFI System Partition, and copies Limine, the kernel, and the system apps from the USB drive to internal storage.
> ```
