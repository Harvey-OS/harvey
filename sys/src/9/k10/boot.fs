#!/boot/rc -m /boot/rcmain
# boot script for file servers, including standalone ones
path=(/boot /amd64/bin /rc/bin .)
echo -n Greetings professor Falken
exec /boot/rc -m/boot/rcmain -i

