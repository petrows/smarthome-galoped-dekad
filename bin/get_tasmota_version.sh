#!/bin/bash
# Extract Tasmota version from tasmota_version.h
# The version is stored as a hex uint32: 0xMMmmppbb (Major.minor.patch.build)
# Example: 0x0F030003 -> 15.3.0.3

VERSION_FILE="firmware-wifi/tasmota/tasmota/include/tasmota_version.h"

# Allow overriding the file path via argument
if [ -n "$1" ]; then
    VERSION_FILE="$1"
fi

if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: Version file not found: $VERSION_FILE" >&2
    exit 1
fi

# Extract the hex value
HEX=$(grep -oP 'TASMOTA_VERSION\s*=\s*\K0x[0-9A-Fa-f]+' "$VERSION_FILE")

if [ -z "$HEX" ]; then
    echo "Error: Could not find TASMOTA_VERSION in $VERSION_FILE" >&2
    exit 1
fi

# Convert hex to decimal
DEC=$((HEX))

# Extract each byte
MAJOR=$(( (DEC >> 24) & 0xFF ))
MINOR=$(( (DEC >> 16) & 0xFF ))
PATCH=$(( (DEC >> 8)  & 0xFF ))
BUILD=$(( DEC & 0xFF ))

echo "${MAJOR}.${MINOR}.${PATCH}.${BUILD}"
