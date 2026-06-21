.section .multiboot
.align 4
.long 0x1BADB002             /* magic */
.long 0x00                   /* flags */
.long -(0x1BADB002 + 0x00)   /* checksum */

.section .text
.global _start
.extern kernel_main

_start:
    /* Set up a stack */
    mov $stack_top, %esp

    /* Call the kernel main */
    call kernel_main

    /* Hang if kernel_main returns */
    cli
.hang:
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB */
stack_top:
