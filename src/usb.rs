use core::num::NonZeroUsize;
use xhci::accessor::Mapper;

#[derive(Clone)]
struct MemoryMapper;

impl Mapper for MemoryMapper {
    unsafe fn map(&mut self, phys_base: usize, _bytes: usize) -> core::num::NonZeroUsize {
        NonZeroUsize::new_unchecked(phys_base)
    }

    fn unmap(&mut self, _virt_base: usize, _bytes: usize) {
        // No-op for now
    }
}

#[allow(dead_code)]
pub fn init_xhci(base_address: usize) {
    let mapper = MemoryMapper;

    let registers = unsafe { xhci::Registers::new(base_address, mapper.clone()) };

    let cap = registers.capability;
    crate::serial_println!(
        "xHCI Capability Registers Length: {}",
        cap.caplength.read_volatile().get()
    );

    let op = registers.operational;
    let usbsts = op.usbsts.read_volatile();
    crate::serial_println!("xHCI Status (halted?): {}", usbsts.hc_halted());
}
