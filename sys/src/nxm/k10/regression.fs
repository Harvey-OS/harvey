#!/boot/rc -m /boot/rcmain
/boot/bind /boot /bin
echo Morning
bind -a . /bin
bind '#p' /proc
bind '#d' /fd
ramfs -s
mount -bc /srv/ramfs /
echo badsyscall && badsyscall
echo float && float
echo sysstatread && sysstatread
echo reboot > /dev/reboot
