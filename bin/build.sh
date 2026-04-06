#!/bin/bash -xe

CHIP=$1
BUILD_ENV=$2
VARIANT=$3

if [[ -z "$CHIP" ]] || [[ -z "$BUILD_ENV" ]] || [[ -z "$VARIANT" ]]; then
    echo "Usage: $0 <chipset> <env> <variant>"
    echo "Chipset: esp32, esp32c6 (fw name only)"
    echo "Env: tasmota32, tasmota32c6 (build arg)"
    echo "Variant: fw variant (fw name only)"
    exit 1
fi

echo "Building firmware for cipset: $CHIP, env: $BUILD_ENV, variant: $VARIANT"

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

# Build firmware
platformio run -e $BUILD_ENV

cp .pio/build/$BUILD_ENV/firmware.bin ../${FW_NAME}.bin
cd ..
tar -czf ${FW_NAME}.tar.gz ${FW_NAME}.bin

echo "Built firmware: firmware-wifi/${FW_NAME}.tar.gz"
