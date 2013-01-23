#!/boot/rc -m /boot/rcmain
/boot/bind /boot /bin
echo Morning
bind -a . /bin
bind '#p' /proc
bind '#d' /fd
bind -a '#Z' /dev
ramfs -s
mount -bc /srv/ramfs /
echo badsyscall && badsyscall
echo float && float
echo frexp && frexp
echo sysstatread && sysstatread
#This will break jenkins but is useful for testing
#echo -n V > /dev/regressctl
echo -n 0x8000000 >/dev/malloc
echo reboot > /dev/reboot
