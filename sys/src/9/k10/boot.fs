#!/boot/rc -m /boot/rcmain
# boot script for file servers, including standalone ones
/boot/echo 'Booting'
#/boot/listen1 -t -v tcp!*!1522  /boot/tty /boot/rc -m /boot/rcmain -i &
#/boot/tty -f'#t/eia0' /boot/rc -m/boot/rcmain -i &
/boot/tty -f'#t/eia0' /boot/boot2
#exec /boot/console /boot/tty /boot/rc -m/boot/rcmain -i
#exec /boot/console /boot/tty /boot/boot2
