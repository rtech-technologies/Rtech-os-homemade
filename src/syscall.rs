use crate::userland::GDT;
use x86_64::registers::model_specific::{LStar, SFMask, Star};
use x86_64::registers::rflags::RFlags;
use x86_64::VirtAddr;

pub fn init() {
    let selectors = &GDT.1;

    Star::write(
        selectors.user_code_selector,
        selectors.user_data_selector,
        selectors.code_selector,
        selectors.data_selector,
    )
    .unwrap();

    LStar::write(VirtAddr::new(syscall_handler as u64));
    SFMask::write(RFlags::INTERRUPT_FLAG);

    use x86_64::registers::model_specific::Efer;
    use x86_64::registers::model_specific::EferFlags;

    unsafe {
        Efer::update(|flags| flags.insert(EferFlags::SYSTEM_CALL_EXTENSIONS));
    }
}

#[naked]
extern "C" fn syscall_handler() {
    unsafe { core::arch::asm!("sysretq", options(noreturn)) }
}
