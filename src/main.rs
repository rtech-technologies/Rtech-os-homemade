#![no_std]
#![no_main]
#![feature(abi_x86_interrupt)]
#![feature(alloc_error_handler)]
#![feature(naked_functions)]

extern crate alloc;

mod allocator;
mod interrupts;
mod memory;
mod ramdisk;
mod script;
mod serial;
mod syscall;
mod usb;
mod userland;
mod vga_buffer;

use bootloader::{entry_point, BootInfo};
use core::panic::PanicInfo;
use x86_64::VirtAddr;

entry_point!(kernel_main);

fn kernel_main(boot_info: &'static BootInfo) -> ! {
    println!("Hello World from VGA!");
    serial_println!("Hello World from Serial!");

    userland::init_gdt();
    interrupts::init_idt();
    unsafe { interrupts::PICS.lock().initialize() };
    x86_64::instructions::interrupts::enable();

    syscall::init();

    let phys_mem_offset = VirtAddr::new(boot_info.physical_memory_offset);
    let mut mapper = unsafe { memory::init(phys_mem_offset) };
    let mut frame_allocator =
        unsafe { memory::BootInfoFrameAllocator::init(&boot_info.memory_map) };

    allocator::init_heap(&mut mapper, &mut frame_allocator).expect("heap initialization failed");

    println!("Interrupts & Memory enabled");

    let script_content = b"print \"Hello from the script engine!\" print 12345";
    ramdisk::RAMDISK
        .lock()
        .create_file("hello.rsl", script_content);

    if let Some(content) = ramdisk::RAMDISK.lock().read_file("hello.rsl") {
        if let Ok(s) = core::str::from_utf8(content) {
            println!("Executing hello.rsl...");
            serial_println!("Executing hello.rsl...");
            script::execute(s);
        }
    }

    println!("Dropping to idle loop...");
    loop {
        x86_64::instructions::hlt();
    }
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    serial_println!("{}", info);
    loop {}
}

#[alloc_error_handler]
fn alloc_error_handler(layout: alloc::alloc::Layout) -> ! {
    panic!("allocation error: {:?}", layout)
}
