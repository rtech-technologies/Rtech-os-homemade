CC       = x86_64-w64-mingw32-gcc

CFLAGS   = -ffreestanding -fno-stack-protector -fno-builtin \
           -fno-asynchronous-unwind-tables -fno-unwind-tables \
           -mno-red-zone -Wall -Wextra

TARGET      = build/BOOTX64.EFI
OBJ         = build/main.o
ISO         = os.iso
IMG_TMP     = fat_disk.img
ISO_ROOT    = build/iso-root
STARTUP_RSL = startup.rsl
INCLUDE_DIR = include

MKFS_FAT ?= /usr/sbin/mkfs.fat

OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS ?= /usr/share/OVMF/OVMF_VARS_4M.fd
OVMF_VARS_RUN = build/OVMF_VARS.fd

.PHONY: all run clean

all: $(ISO)

$(OBJ): main.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) -nostdlib \
		-Wl,--subsystem,10 \
		-Wl,-e,efi_main \
		-Wl,--no-undefined \
		$< -o $@

$(ISO): $(TARGET)
	@test -f "$(STARTUP_RSL)" || { \
		echo "Missing $(STARTUP_RSL); create it beside the Makefile."; exit 1; \
	}
	@test -x "$(MKFS_FAT)" || { \
		echo "FAT formatter not found at $(MKFS_FAT)"; exit 1; \
	}
	@command -v mmd >/dev/null 2>&1 || { \
		echo "mtools missing; install with: apt install mtools"; exit 1; \
	}
	@command -v mcopy >/dev/null 2>&1 || { \
		echo "mtools missing; install with: apt install mtools"; exit 1; \
	}
	@command -v xorriso >/dev/null 2>&1 || { \
		echo "xorriso missing; install with: apt install xorriso"; exit 1; }

	rm -f "$(IMG_TMP)" "$(ISO)"
	rm -rf "$(ISO_ROOT)"
	mkdir -p "$(ISO_ROOT)/EFI/BOOT"

	dd if=/dev/zero of="$(IMG_TMP)" bs=1M count=64 status=none
	"$(MKFS_FAT)" -F 32 "$(IMG_TMP)"

	mmd -i "$(IMG_TMP)" ::/EFI
	mmd -i "$(IMG_TMP)" ::/EFI/BOOT
	mmd -i "$(IMG_TMP)" ::/bin
	mmd -i "$(IMG_TMP)" ::/bin/include

	mcopy -i "$(IMG_TMP)" "$(TARGET)" ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i "$(IMG_TMP)" "$(STARTUP_RSL)" ::/startup.rsl
	mcopy -i "$(IMG_TMP)" "chell_ap.cfg" ::/chell_ap.cfg

	@if [ -d "$(INCLUDE_DIR)" ] && \
	    [ -n "$$(find "$(INCLUDE_DIR)" -mindepth 1 -print -quit)" ]; then \
		mcopy -s -i "$(IMG_TMP)" "$(INCLUDE_DIR)"/* ::/bin/include/; \
	fi

	cp "$(TARGET)" "$(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI"
	cp "$(IMG_TMP)" "$(ISO_ROOT)/fat_disk.img"

	xorriso -as mkisofs \
		-R -J -V UEFI_OS \
		-eltorito-alt-boot \
		-e fat_disk.img \
		-no-emul-boot \
		-o "$(ISO)" "$(ISO_ROOT)"

	@echo "Created $(ISO)"

run: $(ISO)
	@test -f "$(OVMF_CODE)" || { \
		echo "OVMF code image not found: $(OVMF_CODE)"; exit 1; \
	}
	@test -f "$(OVMF_VARS)" || { \
		echo "OVMF variables image not found: $(OVMF_VARS)"; exit 1; \
	}
	cp "$(OVMF_VARS)" "$(OVMF_VARS_RUN)"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file="$(OVMF_CODE)" \
		-drive if=pflash,format=raw,file="$(OVMF_VARS_RUN)" \
		-drive if=none,format=raw,file="$(IMG_TMP)",id=usbdisk \
		-device qemu-xhci,id=xhci \
		-device usb-storage,drive=usbdisk,bus=xhci.0 \
		-device usb-kbd,bus=xhci.0 \
		-display none \
		-m 256M \
		-serial file:build/serial.log

clean:
	rm -rf build "$(ISO)" "$(IMG_TMP)"