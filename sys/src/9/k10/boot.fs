#!/boot/rc -m /boot/rcmain
#/boot/echo Morning
# boot script for file servers, including standalone ones
path=(/boot /$cputype/bin /rc/bin .)
bind -a /boot /bin
bind -b '#c' /dev
bind -b '#ec' /env
bind -b '#e' /env
bind -b '#s' /srv
bind -a /boot /
prompt=('cpu% ' '	')
exec /boot/rc -m /boot/rcmain -i
