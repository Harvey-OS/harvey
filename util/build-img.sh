#!/bin/bash

# Build a qcow2 image for loading in qemu

SRC=$HARVEY/util/syslinux-bios
DEST=$HARVEY/harvey.qcow2

echo "Creating harvey image $DEST"

virt-make-fs --partition=mbr --type=ext4 --format=qcow2 --size=1G $SRC $DEST
guestfish -a "$DEST" <<EOF
launch
mount /dev/sda1 /
part-set-bootable /dev/sda 1 true
extlinux /
copy-file-to-device /mbr.bin /dev/sda size:440
copy-in $HARVEY/sys/src/9/amd64/harvey.32bit /
rename /harvey.32bit /harvey
rm /mbr.bin
EOF

echo "Done"
