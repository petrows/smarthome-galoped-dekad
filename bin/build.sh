#!/bin/bash -xe

VARIANT=$1

if [ -z "$VARIANT" ]; then
    echo "Usage: $0 <variant>"
    echo "Available variants: rgb, retro"
    exit 1
fi

echo "Building firmware for variant: $VARIANT"

cd firmware/tasmota

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
cp ../user_config_hw_${VARIANT}.h tasmota/user/user_config_hw.h

# Build firmware
platformio run -e tasmota32

cp .pio/build/tasmota32/firmware.bin galoped-dekad-firmware-${VARIANT}.bin
tar -czf ../galoped-dekad-firmware-${VARIANT}.tar.gz galoped-dekad-firmware-${VARIANT}.bin
rm -rf galoped-dekad-firmware*

echo "Built firmware: firmware/galoped-dekad-firmware-${VARIANT}.tar.gz"
