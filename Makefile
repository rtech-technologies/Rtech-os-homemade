# Basic Makefile for RSL-OS kernel

CC := gcc
LD := ld
AS := as

CFLAGS := -Wall -Wextra -O2 -pipe \
    -ffreestanding -fno-stack-protector -fno-stack-check \
    -fno-lto -fPIE -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-red-zone
CPPFLAGS := -Ikernel

LDFLAGS := -nostdlib -pie -z text -z max-page-size=0x1000 -T kernel/linker.ld

KERNEL_SRC := $(wildcard kernel/*.c)
KERNEL_OBJ := $(KERNEL_SRC:.c=.o)
KERNEL_BIN := build/kernel.elf
ISO_IMAGE  := build/rsl-os.iso

LIMINE_GIT := https://github.com/limine-bootloader/limine.git
LIMINE_BRANCH := v8.x-binary
LIMINE_DIR := limine-bin

.PHONY: all clean iso

all: $(ISO_IMAGE)

kernel/%.o: kernel/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJ) kernel/linker.ld
	mkdir -p build
	$(LD) $(KERNEL_OBJ) $(LDFLAGS) -o $@

$(LIMINE_DIR):
	git clone $(LIMINE_GIT) --branch $(LIMINE_BRANCH) --depth=1 $(LIMINE_DIR)
	make -C $(LIMINE_DIR)

iso: $(ISO_IMAGE)

$(ISO_IMAGE): $(KERNEL_BIN) $(LIMINE_DIR) limine.conf
	rm -rf build/iso_root
	mkdir -p build/iso_root/boot
	cp $(KERNEL_BIN) build/iso_root/boot/
	cp limine.conf build/iso_root/boot/
	mkdir -p build/iso_root/boot/limine
	cp -v $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-uefi-cd.bin build/iso_root/boot/limine/
	cp -v $(LIMINE_DIR)/BOOTX64.EFI $(LIMINE_DIR)/BOOTIA32.EFI build/iso_root/boot/limine/
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		build/iso_root -o $(ISO_IMAGE)
	$(LIMINE_DIR)/limine bios-install $(ISO_IMAGE)

clean:
	rm -f $(KERNEL_OBJ)
	rm -rf build
	rm -rf $(LIMINE_DIR)
