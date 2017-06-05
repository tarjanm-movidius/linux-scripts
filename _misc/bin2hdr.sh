#!/bin/sh

# Binary to C static array header definition converter

[ $# -lt 1 ] && echo "Usage: `basename $0` <binary_to_convert> [array_name]" && echo " If array_name is specified file will be saved as <array_name>.h (defaults to boot_bin)" && exit 1
[ ! -e "$1" ] && echo "Error: file '$1' not found" && exit 2

HDR="boot_bin"
[ $# -gt 1 ] && HDR="$2"
echo "$HDR" | grep -q '\W' && echo "Invalid array_name '$HDR'" && exit 3
[ -e "$HDR.h" ] && mv -v "$HDR.h" "$HDR.h~"

echo "const unsigned char $HDR[] = { \\" > "$HDR.h"
hexdump -v -e '16/1 " 0x%02X," " \\\n"' "$1" | sed 's/0x  , //g' | head -c -4 >> "$HDR.h"
echo " };" >> "$HDR.h"
