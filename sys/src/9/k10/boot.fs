#!/boot/rc -m /boot/rcmain
# boot script for file servers, including standalone ones
path=(/boot /bin /amd64/bin /rc/bin .)
prompt=('harvey@cpu% ' '	')
echo 'Hello, I am Harvey :-)'
bind -a /boot /bin
bind -a '#I' /net
bind -a '#l0' /net
bind -a '#p' /proc
bind -a '#s' /srv
ipconfig
/boot/listen1 -t -v tcp!*!1522  /boot/tty /boot/rc -m /boot/rcmain -i &
/boot/tty -f'#t/eia0' /boot/rc -m/boot/rcmain -i &
exec /boot/console /boot/tty /boot/rc -m/boot/rcmain -i
