#!/bin/sh

if [ -e "./cpio" ]; then
	[ -e "./cpio~" ] && rm -r cpio~
	mv cpio cpio~
fi
mkdir cpio
cd cpio
zcat /media/mmcblk0p1/core-image-minimal-initramfs-clanton.cpio.gz | cpio -idmv
echo "Updating kernel modules"
rm -r lib/modules/3.8.7
cp -r /lib/modules/3.8.7 lib/modules/
echo "Writing new initrd"
find . | cpio -o -H newc | gzip -9 > /media/mmcblk0p1/core-image-minimal-initramfs-clanton.cpio.gz
sync
