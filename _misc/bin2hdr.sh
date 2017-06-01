#!/bin/sh

# Binary to C static array header definition convereter

[ $# -lt 1 ] && echo "Usage: `basename $0` <binary_to_convert> [hex_header_name.h]" && exit 1
[ ! -e "$1" ] && echo "Error: file '$1' not found" && exit 1

HDR="boot_bin.h"
[ $# -gt 1 ] && HDR="$2"
[ -e "$HDR" ] && mv "$HDR" "$HDR~"

echo "const unsigned char boot_bin[] = { \\" > "$HDR"
hexdump -v -e '16/1 " 0x%02X," " \\\n"' "$1" | sed 's/0x  , //g' | head -c -4 >> "$HDR"
echo " };" >> "$HDR"
