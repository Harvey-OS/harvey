#!/bin/bash
set -e
set -x

# this uses a coreboot built for emulation/q35, with a tianocore payload. 
# you MUST use -machine q35 for coreboot to work.
qemu-system-x86_64  -machine q35 -net user -net nic,model=virtio -m 1024  -drive if=none,id=stick,format=raw,file=plan9-usb.img -device usb-ehci,id=ehci  -device usb-storage,bus=ehci.0,drive=stick -boot menu=on -bios coreboot.rom -serial stdio $*
