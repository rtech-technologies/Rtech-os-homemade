#!/bin/bash
set -e

# Build the ISO if it doesn't exist
make all

OVMF_PATH=""

# Standard paths to search for OVMF
SEARCH_PATHS=(
    "/usr/share/OVMF/OVMF.fd"
    "/usr/share/ovmf/OVMF.fd"
    "/usr/share/edk2-ovmf/x64/OVMF.fd"
    "OVMF.fd"
)

# Look for an existing OVMF firmware
for p in "${SEARCH_PATHS[@]}"; do
    if [ -f "$p" ]; then
        OVMF_PATH="$p"
        break
    fi
done

# If not found, download it locally
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
    $DISPLAY_OPT
