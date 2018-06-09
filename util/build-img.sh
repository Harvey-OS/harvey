#!/bin/bash

# Build a qcow2 image for loading in qemu

SRC_IMG=$HARVEY/util/img/syslinux-bios
SRC_SYSLINUX=$SRC_IMG/syslinux
SRC_MBR=$SRC_IMG/mbr.bin
SRC_KERNEL=$HARVEY/sys/src/9/amd64/harvey.32bit

DEST=$HARVEY/harvey.qcow2

echo "Creating harvey image $DEST"
rm -f $DEST

guestfish <<EOF
disk-create $DEST qcow2 1G
add-drive $DEST

launch

part-init /dev/sda mbr
part-add /dev/sda p 2048 104447
part-add /dev/sda p 104448 1992704
part-set-mbr-id /dev/sda 2 57
mke2fs /dev/sda1
part-set-bootable /dev/sda 1 true

mount /dev/sda1 /
extlinux /
copy-in $SRC_SYSLINUX /
copy-in $SRC_KERNEL /
rename /harvey.32bit /harvey

copy-in $SRC_MBR /
copy-file-to-device /mbr.bin /dev/sda size:440
rm /mbr.bin
EOF

echo "Done"
