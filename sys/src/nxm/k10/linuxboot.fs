#!/boot/rc -m /boot/rcmain
/boot/bind /boot /bin
echo Morning
bind -a /$cputype/bin /bin
bind -a /rc/bin /bin
bind -a . /bin
bind '#p' /proc
bind '#d' /fd
ramfs -s
mount -bc /srv/ramfs /
mkdir -p /sys/lib/acid
bind /bin /sys/lib/acid
exec /boot/rc -m/boot/rcmain -i
