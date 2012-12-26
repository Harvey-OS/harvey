#!/boot/rc -m /boot/rcmain
/boot/bind /boot /bin
echo Morning
bind -a . /bin
bind '#p' /proc
bind '#d' /fd
ramfs -s
mount -bc /srv/ramfs /
badsyscall
float
sysstatread
echo reboot > /dev/reboot
