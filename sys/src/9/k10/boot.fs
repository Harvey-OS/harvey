#!/boot/rc -m /boot/rcmain
/boot/echo Morning
# boot script for file servers, including standalone ones
path=(/boot /$cputype/bin /rc/bin .)
exec /boot/rc -m/boot/rcmain -i
