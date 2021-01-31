#!/bin/bash

# Build a qcow2 image for loading in qemu

# Syslinux should already be installed on the system since it's a
# dependency of guestfish. We want to use the system syslinux binaries
# which match the extlinux command below.
SRC_SYSLINUX=/usr/lib/syslinux/bios
SRC_MBR=/usr/lib/syslinux/bios/mbr.bin
SRC_CFG=$HARVEY/util/img/syslinux-bios/syslinux/syslinux-usb.cfg
SRC_KERNEL=$HARVEY/sys/src/9/amd64/harvey.32bit


FORMAT=qcow2
if [ "$1" = "-f" ]; then
	FORMAT=$2
fi
if [ "$FORMAT" = "" ]; then
	echo "usage: $0 [-f format]" 1>&2
	exit 2
fi

DEST=$HARVEY/harvey.$FORMAT
echo "Creating harvey image $DEST"
rm -f $DEST

guestfish <<EOF
disk-create $DEST $FORMAT 3G
add-drive $DEST

launch

part-init /dev/sda mbr
part-add /dev/sda p 2048 104447
part-add /dev/sda p 104448 6291455
part-set-mbr-id /dev/sda 2 57
mke2fs /dev/sda1
part-set-bootable /dev/sda 1 true

mount /dev/sda1 /
extlinux /
copy-in $SRC_SYSLINUX /
rename /bios /syslinux
copy-in $SRC_CFG /syslinux
rename /syslinux/syslinux-usb.cfg /syslinux/syslinux.cfg
copy-in $SRC_KERNEL /
rename /harvey.32bit /harvey

copy-in $SRC_MBR /
copy-file-to-device /mbr.bin /dev/sda size:440
rm /mbr.bin
EOF

echo "Done"
