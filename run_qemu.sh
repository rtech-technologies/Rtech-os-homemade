#!/bin/bash
set -e

# Build the ISO if it doesn't exist
make all

# Create a virtual FAT32 image for AHCI if it doesn't exist
if [ ! -f "disk.img" ]; then
    echo "Creating virtual FAT32 drive disk.img..."
    dd if=/dev/zero of=disk.img bs=1M count=32
    mkfs.fat -F 32 disk.img
    # Use mcopy from mtools to put a file on it
    echo "Hello from FAT32 Disk!" > test.txt
    mcopy -i disk.img test.txt ::/
    rm test.txt
fi

OVMF_PATH=""

SEARCH_PATHS=(
    "/usr/share/OVMF/OVMF.fd"
    "/usr/share/ovmf/OVMF.fd"
    "/usr/share/edk2-ovmf/x64/OVMF.fd"
    "OVMF.fd"
)

for p in "${SEARCH_PATHS[@]}"; do
    if [ -f "$p" ]; then
        OVMF_PATH="$p"
        break
    fi
done

if [ -z "$OVMF_PATH" ]; then
    echo "OVMF.fd not found in standard paths. Downloading..."
    mkdir -p .cache
    OVMF_PATH=".cache/OVMF.fd"
    if [ ! -f "$OVMF_PATH" ]; then
        curl -L -o "$OVMF_PATH" "https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd"
    fi
fi

echo "Using OVMF at: $OVMF_PATH"

DISPLAY_OPT=""
if [ -z "$DISPLAY" ]; then
    DISPLAY_OPT="-display none"
fi

# Run QEMU
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -bios "$OVMF_PATH" \
    -cdrom build/rsl-os.iso \
    -boot d \
    -drive id=disk,file=disk.img,if=none,format=raw \
    -device ahci,id=ahci \
    -device ide-hd,drive=disk,bus=ahci.0 \
    $DISPLAY_OPT
