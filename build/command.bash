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
expect -re "vgasize .*:"
send "1280x1024x32\n"
expect -re "monitor is .*:"
send "vesa\n"

# Mount the host's fileserver
expect -exact "term% "
send "9fs tcp!10.0.2.2!5640 /n/harvey\n"

# Now run a command
expect -exact "term% "
send "ls -l /n/harvey/\n"

expect -exact "term% "
send "ls -l /n/harvey/build\n"

expect -exact "term% "
send "ls -l /n/harvey/build/go\n"

# make go available in the image
expect -exact "term% "
# this will make a /go appear, along with all the other stuff we don't want,
# but hey ...
# one option is to great build/go/go and build in there, and then bind that
# in /n/harvey/build/go in /, in which case there will be a /go.
send "bind -a /n/harvey/build /\n"
expect -exact "term% "
send "ls -l /\n"
expect -exact "term% "
send "ls -l /go\n"

# and shut down
expect -exact "term% "
send "fshalt\n"
expect -exact "done halting"
exit
EOF
