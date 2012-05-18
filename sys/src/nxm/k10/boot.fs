#!/boot/rc -m /boot/rcmain
/boot/bind /boot /bin
echo Morning
bind -a /$cputype/bin /bin
bind -a /rc/bin /bin
bind -a . /bin
bind '#p' /proc
exec /boot/rc -m/boot/rcmain -i
