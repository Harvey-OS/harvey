This folder is set up to allow netbooting using pxeserver from u-root.

Pxeserver hosts the kernel using http, and the pxelinux files using tftp.

Assumptions:
- The following ports are open on your firewall: 67, 68, 69 (tftp), 80 (http server), 5640 (ufs)
- You've successfully built harvey.

---------------------
Create the file $HARVEY/cfg/pxe/tftpboot/pxelinux.cfg/default
The file should contain (change the IP address to that of the machine running pxeserver and ufs):

DEFAULT harvey
LABEL harvey
  KERNEL mboot.c32
  APPEND http://192.168.0.19/harvey.32bit service=cpu nobootprompt=tcp maxcores=1024 fs=192.168.0.19 auth=192.168.0.19 nvram=/boot/nvram nvrlen=512 nvroff=0 acpiirq=1

---------------------

Fetch and build pxeserver:
  go get github.com/u-root/u-root
  go install github.com/u-root/u-root/cmds/exp/pxeserver

Run pxeserver to host pxelinux and harvey (make sure you change the interface and IP to match the one on your server):
  sudo pxeserver \
      -tftp-dir $HARVEY/cfg/pxe/tftpboot/ \
      -http-dir $HARVEY/cfg/pxe/tftpboot/ \
      -bootfilename lpxelinux.0 \
      -interface enp0s31f6 \
      -ip 192.168.0.19

Run ufs to host the harvey files over 9p:
  $HARVEY/util/ufs -root $HARVEY
