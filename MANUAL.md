# Rtech-OS Kernel Development Manual

Welcome to your custom operating system! The foundation is ready. This manual explains how to use the drivers I've built for you and how to expand the system with advanced libraries like Nuklear and CherryUSB.

---

## 1. Easy API Reference (The "Cheatsheet")

You can call these functions anywhere in your `kernel_main` loop.

### Screen (VGA Text Mode)
* `clear_screen()`: Wipes the screen black.
* `kprint("Message")`: Prints text at the current cursor position.
* `kprint_at("Message", x, y)`: Prints text at specific coordinates.
* `kprint_at_color("Message", x, y, color)`: Prints with custom colors (e.g., `RED_ON_WHITE`).
* `kprint_backspace()`: Deletes the last character.

### Input (Keyboard)
* `kernel_get_char()`: Returns the next character in the buffer. Returns `0` if no key was pressed.
  * *Tip:* Use this in a loop to build a command prompt or text editor.

### Time (Timer)
* `kernel_sleep(ticks)`: Pauses the system. (50 ticks ≈ 1 second by default).

---

## 2. Your "Main" Playground

I have set up `src/kernel/kernel.c` as your starting point.

```c
void kernel_main() {
    // [System Inits - DO NOT TOUCH]
    init_gdt();
    init_idt();
    clear_screen();
    init_timer(50);
    init_keyboard();
    __asm__ volatile("sti");

    // [YOUR CODE STARTS HERE]
    kprint("Welcome to My OS!\n");

    while(1) {
        char c = kernel_get_char();
        if (c != 0) {
            // Do something with input!
        }
    }
}
```

---

## 3. How to Add Nuklear (GUI Library)

Nuklear is a "header-only" library, which makes it perfect for bare-metal.

1.  **Download:** Put `nuklear.h` in `src/include/`.
2.  **Implementation:** Create a `src/drivers/gui.c`. Since Nuklear is "Immediate Mode", you need to provide it a "Back-end" (a way to draw pixels).
3.  **Drawing:** Nuklear will give you commands like "Draw Rect". You will need to upgrade my **Screen Driver** from Text Mode (80x25 characters) to a **Framebuffer Driver** (e.g., 800x600 pixels) to see the GUI.
4.  **Input:** Pipe `kernel_get_char()` data into Nuklear's input handling functions.

---

## 4. How to Add CherryUSB (USB Stack)

CherryUSB is designed for embedded/bare-metal systems.

1.  **Porting:** You need to implement the "HAL" (Hardware Abstraction Layer) for CherryUSB. This means writing functions that read/write to your USB Controller's MMIO addresses.
2.  **Memory:** CherryUSB requires a heap (malloc). You will need to implement a simple `kmalloc` in a new file `src/kernel/mem.c`.
3.  **Integration:**
    * Include CherryUSB source in your `Makefile`.
    * Call `usb_host_init()` or `usb_device_init()` after `init_idt()`.

---

## 5. Helpful Tips for Success

*   **Compilation:** Just type `make`. It produces `kernel.bin`.
*   **No Standard Libs:** You cannot use `<stdio.h>` or `<stdlib.h>`. Use the functions in `src/include/utils.h` instead (I've provided `memory_copy`, `int_to_ascii`, etc.).
*   **Bare Metal is Raw:** If you write to the wrong memory address, the system will just hang. Use the `kprint` function everywhere to debug!

Have fun building your OS!
