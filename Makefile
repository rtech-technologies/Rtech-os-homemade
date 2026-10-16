.PHONY: all run clean iso

all: iso

run: iso
	qemu-system-x86_64 -drive format=raw,file=target/x86_64-kernel/debug/bootimage-kernel.bin -display none -serial stdio -device isa-debug-exit,iobase=0xf4,iosize=0x04

install:
	sudo apt-get update && sudo apt-get install -y qemu-system-x86
	cargo install bootimage --version 0.10.3 || true

iso:
	cargo bootimage

clean:
	cargo clean
