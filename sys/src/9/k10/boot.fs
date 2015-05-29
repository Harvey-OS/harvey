#!/boot/rc -m /boot/rcmain
# boot script for file servers, including standalone ones
path=(/boot /amd64/bin /rc/bin .)
prompt=('harvey@cpu% ' '	')
echo
echo 'Hello, I am Harvey :-)'
echo
bind -a '#I' /net
bind -a '#l0' /net
bind -a '#p' /proc
bind -a '#d' /fd
exec /boot/rc -m/boot/rcmain -i

