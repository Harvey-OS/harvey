#!/boot/rc -m /boot/rcmain
# boot script for file servers, including standalone ones
path=(/boot /$cputype/bin /rc/bin .)
fn diskparts {
	# set up any /dev/sd partitions
	for(disk in /dev/sd*) {
		if(test -f $disk/data && test -f $disk/ctl)
			fdisk -p $disk/data >$disk/ctl >[2]/dev/null
		if(test -f $disk/plan9)
			parts=($disk/plan9*)
		if not
			parts=($disk/data)
		for(part in $parts)
			if(test -f $part)
				prep -p $part >$disk/ctl >[2]/dev/null
	}
}

cd /boot
echo -n boot...
if (! test -e /env/vmpc)
	vmpc=23			# % of free memory for venti
cp '#r/rtc' '#c/time'
bind -a '#I0' /net
bind -a '#l0' /net
bind -a '#¤' /dev
bind -a '#S' /dev
bind -a '#k' /dev
bind -a '#æ' /dev
bind -a '#u' /dev
bind '#p' /proc
bind '#d' /fd
bind -c '#s' /srv
# bind -a /boot /

# start usb for keyboard, disks, etc.
if (test -e /dev/usb/ctl) {
	echo -n usb...
	usbd
}
if not if (test -e /dev/usb0) {
	echo -n old usb...
	usbd
	if (test -e '#m/mouse')
		kb -a2
	if not
		kb -k
	disk -l -s usbdisk -m /mnt	# mounts on /mnt/<lun>
}

echo -n disks...
if(! ~ $dmaon no)
	for (ctl in /dev/sd[C-H]?/ctl)
		if (test -e $ctl)
			echo 'dma on' >$ctl

diskparts

# set up any fs(3) partitions (HACK)
# don't match AoE disks, as those may be shared.
if (test ! -e /env/fscfg)
	fscfg=`{ls -d /dev/sd[~e-h]?/fscfg >[2]/dev/null | sed 1q}
if (~ $#fscfg 1 && test -r $fscfg)
	zerotrunc <$fscfg | read -m >/dev/fs/ctl

# figure out which arenas and fossil partitions to use.
# don't match AoE disks, as those may be shared.
if(! test -e /env/arena0){
	if (test -e	/dev/fs/arena0)
		arena0=	/dev/fs/arena0
	if not if (test -e	/dev/sd[~e-h]?/arena0)
		arena0=		/dev/sd[~e-h]?/arena0
	if not
		arena0=/dev/null
}
if (test -e	/dev/fs/fossil)
	fossil=	/dev/fs/fossil
if not if (test -e	/dev/sd[~e-h]?/fossil)
	fossil=		/dev/sd[~e-h]?/fossil
if not
	fossil=/dev/null

#
# the local disks are now sorted out.
# set up the network, auth, venti and fossil.
#

echo -n ip...
if (~ $#ip 1 && ! ~ $ip '') {
	# need to supply ip, ipmask and ipgw in plan9.ini to use this
	ipconfig -g $ipgw ether /net/ether0 $ip $ipmask
	echo 'add 0 0 '^$ipgw >>/net/iproute
}
if not
	ipconfig
switch (`{sed '/\.(0|255)[	 ]/d' /net/ipselftab}) {
case 204.178.31.*
	echo 'add 135.104.9.0 255.255.255.0 204.178.31.10' >>/net/iproute
}
ipconfig loopback /dev/null 127.1

# local hackery: add extra sr luns
if (test -e /dev/aoe/1.1 && ! test -e /dev/sdf0)
	echo config switch on spec f type aoe//dev/aoe/1.1 >/dev/sdctl
if (test -e /dev/aoe/1.2 && ! test -e /dev/sdg0)
	echo config switch on spec g type aoe//dev/aoe/1.2 >/dev/sdctl
diskparts

# so far we're using the default key from $nvram, usually
# for insideout.plan9.bell-labs.com on outside machines,
# and have mounted our root over the net, if running diskless.
# factotum always mounts itself (on /mnt by default).

echo -n factotum...
if(~ $#auth 1){
	echo start factotum on $auth
	factotum -sfactotum -S -a $auth
}
if not
	factotum -sfactotum -S
mount -b /srv/factotum /mnt

# if a keys partition exists, add its contents to factotum's
keys=`{ls -d /dev/sd*/keys >[2]/dev/null | sed 1q}
if (~ $#keys 1 && test -r $keys) {
	echo -n add keys...
	zerotrunc <$keys | aescbc -n -d | read -m >/mnt/factotum/ctl
}

# get root from network if fsaddr set in plan9.ini, and bail out here
if (test -e /env/fs) {
	echo -n fs root...
	if(! srv tcp!$fs!564 boot)
		exec ./rc -m/boot/rcmain -i
	if(! mount -c /srv/boot /root)
		exec ./rc -m/boot/rcmain -i
}

# start venti store
if (! ~ $arena0 /dev/null && test -r $arena0) {
	echo -n start venti on $arena0...
	venti=tcp!127.0.0.1!17034
	vcfg=`{ls -d /dev/sd*/venticfg >[2]/dev/null | sed 1q}
	if (~ $#vcfg 1 && test -r $vcfg)
		venti -m $vmpc -c $vcfg
	if not
		venti -m $vmpc -c $arena0
	sleep 10
}
if not if (! test -e /env/venti)
	venti=tcp!135.104.9.33!17034		# local default

# start root fossil, may serve /srv/boot
if (! ~ $fossil /dev/null && test -r $fossil) {
	echo -n root fossil on $fossil...
	fossil -m 2 -f $fossil
	sleep 3
}

#
# normal start up on local fossil root
#

rootdir=/root
rootspec=main/active

# factotum is now mounted in /mnt; keep it visible.
# newns() needs it, among others.

# mount new root
if (test -e /srv/boot)
	srv=boot
if not if (test -e /srv/fossil)
	srv=fossil
if not if (test -e /srv/fsmain)
	srv=fsmain
if not {
	echo cannot find a root in /srv:
	ls -l /srv
}
echo -n mount -cC /srv/$srv $rootdir...
	mount -cC /srv/$srv $rootdir
bind -a $rootdir /

if (test -d $rootdir/mnt)
	bind -ac $rootdir/mnt /mnt
mount -b /srv/factotum /mnt

# standard bin
if (! test -d /$cputype) {
	echo /$cputype missing!
	exec ./rc -m/boot/rcmain -i
}
bind /$cputype/bin /bin
bind -a /rc/bin /bin

# run cpurc
echo cpurc...
path=(/bin . /boot)
/$cputype/init -c

exec ./rc -m/boot/rcmain -i
