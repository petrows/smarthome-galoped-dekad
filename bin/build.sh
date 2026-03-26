#!/bin/bash -xe

CHIP=$1
VARIANT=$2

if [[ -z "$CHIP" ]] || [[ -z "$VARIANT" ]]; then
    echo "Usage: $0 <chipset> <variant>"
    echo "Available chipset: esp32, esp32c6"
    echo "Available variants: rgb, retro, biax"
    exit 1
fi

echo "Building firmware for cipset: $CHIP, variant: $VARIANT"

FW_NAME=galoped-wifi-firmware-${CHIP}-${VARIANT}

cd firmware-wifi/tasmota

# Build safeboot binary (if CI)
# if [ "$CI" = "true" ]; then
#     rm -rf tasmota/user
#     rm -rf tasmota/user_config_override.h
#     platformio run -e tasmota32-safeboot
# fi

# Copy config overrides
cp ../user_config_override.h tasmota/

# Copy HW version config
mkdir -p tasmota/user/
cp ../user_config_${CHIP}-${VARIANT}.h tasmota/user/user_config_hw.h

# Build firmware
platformio run -e tasmota32

cp .pio/build/tasmota32/firmware.bin ../${FW_NAME}.bin
cd ..
tar -czf ${FW_NAME}.tar.gz ${FW_NAME}.bin

echo "Built firmware: firmware-wifi/${FW_NAME}.tar.gz"
