#!/bin/rc

if(~ $#debug 1 && ~ $debug 1)
	flag x +
if not
	debug=0


if(~ $debug yes) echo env...
sysname=gnot
font=/lib/font/bit/lucidasans/typelatin1.7.font

for (i in '#P' '#S' '#f' '#m' '#t' '#v') {
	if(~ $debug yes) echo bind $i
	bind -a $i /dev >/dev/null >[2=1]
}
if(~ $debug yes) echo binddev done

for(disk in /dev/sd??) {
	if(test -f $disk/data && test -f $disk/ctl)
		disk/fdisk -p $disk/data >$disk/ctl >[2]/dev/null
}


for (i in /sys/log/*) {
	if(~ $debug yes) echo bind $i
	bind /dev/null $i
}

if(~ $debug yes) echo bindlog done

bind -a '#l' /net >/dev/null >[2=1]

dossrv
a:
cp /n/a:/plan9.ini /tmp/plan9.orig

# fix devsd nonsense
chmod 664 /dev/sd*/* >[2]/dev/null
chgrp -u glenda /dev/sd*/* >[2]/dev/null

# restore a partial install
if(test -f /n/a:/9inst.cnf)
	cp /n/a:/9inst.cnf /tmp/vars

# make vgadb easier to edit
if(test -f /n/a:/vgadb)
	cp /n/a:/vgadb /lib/vgadb

if(~ $#mouseport 1) {
	aux/mouse $mouseport
	if(~ $#vgasize 1 && ! ~ $vgasize '') {
		vgasize=`{echo $vgasize}
		aux/vga -vip $vgasize >/n/a:/vgainfo.txt
		sleep 2	# wait for floppy to finish
		aux/vga -l $vgasize
		echo -n 'hwaccel off' >'#v/vgactl' >[2]/dev/null
		echo -n 'hwblank off' >'#v/vgactl' >[2]/dev/null
	}
}
home=/usr/$user
cd
. lib/profile
