set -e

# Download Plan 9
if [ ! -e 9legacy.iso ] && [ ! -e 9legacy.iso.bz2 ]; then
  curl -L --fail -O https://github.com/Harvey-OS/harvey/releases/download/9legacy/9legacy.iso.bz2
fi
if [ ! -e 9legacy.iso ]; then
  bunzip2 -k 9legacy.iso.bz2
fi

expect <<EOF
spawn qemu-system-i386 -nographic -net user -net nic,model=virtio -m 1024 -vga none -cdrom 9legacy.iso -boot d
expect -exact "Selection:"
send "2\n"
expect -exact "Plan 9"
sleep 5
expect -timeout 600 "root is from (tcp, local)"
send "local\n"
expect -exact "mouseport is (ps2, ps2intellimouse, 0, 1, 2)\[ps2\]:"
send "ps2intellimouse\n"
expect -exact "vgasize \[640x480x8\]:"
send "1280x1024x32\n"
expect -exact "monitor is \[xga\]:"
send "vesa\n"

# Now run a command
expect -exact "term% "
send "ls\n"

# and shut down
expect -exact "term% "
send "fshalt\n"
expect -exact "done halting"
exit
EOF