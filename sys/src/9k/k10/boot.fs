#!/boot/rc -m /boot/rcmain
# /boot/boot script for file servers, including standalone ones
rfork e
path=(/boot /$cputype/bin /rc/bin .)
cd /boot
echo -n boot...

# set up initial namespace.
# initcode (startboot) has bound #c to /dev, #e & #ec to /env & #s to /srv.
bind -a '#I0' /net
bind -a '#l0' /net
for (dev in '#¤' '#S' '#k' '#æ' '#u')	# auth, sd, fs, aoe, usb
	bind -a $dev /dev >[2]/dev/null
bind '#p' /proc
bind '#d' /fd
# bind -a /boot /

cp '#r/rtc' /dev/time

# start usb for keyboard, disks, etc.
if (test -e /dev/usb/ctl) {
	echo -n usb...
	usbd
}

# give any external caldigit drive time to come on-line, if present.
@ {
	extdisk=/dev/sdE6
	ls $extdisk >/dev/null >[2]/dev/null &
	sleep 1
	if (test -e $extdisk && ~ `{read <$extdisk/ctl} inquiry' 'caldigit*) {
		echo -n await extdisk...
		for (n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)
			if(! test -e $extdisk/data) {
				echo -n .
				sleep 5
			}
		if(! test -e $extdisk/data)
			echo external disk on $extdisk off-line!
	}
}

# make local disk partitions visible
fn diskparts {			# set up any /dev/sd partitions
	# avoid touching sdF1 (no drive), it may hang
	for(disk in `{grep -l '^inquiry..' '#S'/sd*/ctl | sed 's;/ctl$;;'}) {
		echo -n $disk...
		if(test -f $disk/data && test -f $disk/ctl)
			fdisk -p $disk/data >$disk/ctl # >[2]/dev/null
		if(test -f $disk/plan9)
			parts=($disk/plan9*)
		if not
			parts=($disk/data)
		for(part in $parts)
			if(test -f $part)
				prep -p $part >$disk/ctl # >[2]/dev/null
	}
	echo
}
# local hackery for AoE: make visible extra sr luns of shelf 1.
# doesn't use the ip stack, just ethernet.
if (test -e /dev/aoe) {
	echo -n aoe...
	if (test -e /dev/aoe/1.1 && ! test -e /dev/sdf0)
		echo config switch on spec f type aoe//dev/aoe/1.1 >/dev/sdctl
	if (test -e /dev/aoe/1.2 && ! test -e /dev/sdg0)
		echo config switch on spec g type aoe//dev/aoe/1.2 >/dev/sdctl
}
echo -n partitions...
diskparts
fn diskparts

# set up any fs(3) partitions
# don't match AoE disks, as those may be shared.
if (test ! -e /env/fscfg) {
	fscfgs=`{echo /dev/sd[~e-h]?/fscfg}
	if (! ~ $#fscfgs 0)
		fscfg=$fscfgs(1)
}
if (~ $#fscfg 1 && test -r $fscfg) {
	echo reading /dev/fs definitions from $fscfg...
	zerotrunc <$fscfg | read -m >/dev/fs/ctl
}

#
# set up the network.
#
echo -n ip...
if (~ $#ip 1 && ! ~ $ip '') {
	# need to supply ip, ipmask and ipgw in plan9.ini to use this
	ipconfig -g $ipgw ether /net/ether0 $ip $ipmask
	echo 'add 0 0 '^$ipgw >>/net/iproute
}
if not
	ipconfig
ipconfig loopback /dev/null 127.1
# routing example: if outside, add explicit vfw routing to the inside
# switch (`{sed '/\.(0|255)[	 ]/d' /net/ipselftab}) {
# case 135.104.24.*				# new outside
# 	echo 'add 135.104.9.0 255.255.255.0 135.104.24.13' >>/net/iproute
# }

#
# set up auth via factotum, load from secstore & mount network root, if named.
#
# so far we're using the default key from $nvram,
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

# get root from network if fs addr set in plan9.ini.  bail out on error.
if (test -e /env/fs) {
	echo -n $fs on /root...
	if(! srv tcp!$fs!564 boot || ! mount -c /srv/boot /root)
		exec ./rc -m/boot/rcmain -i
}

#
# start venti store if we have arenas (don't search AoE).
# otherwise point to the local venti machine.
#
if(! test -e /env/arena0){
	if (test -e    /dev/fs/arena0)
		arena0=/dev/fs/arena0
	if not if (test -e /dev/sd[~e-h]?/arena0) {
		arena0=    /dev/sd[~e-h]?/arena0
		arena0=$arena0(1)
	}
	if not
		arena0=/dev/no-arenas
}
if (test -r $arena0) {
	echo start venti on $arena0...
	if (! test -e /env/vmpc)
		vmpc=23			# % of free memory for venti
	venti=tcp!127.0.0.1!17034
	vcfg=`{ls -d /dev/sd*/venticfg >[2]/dev/null | sed 1q}
	if (! ~ $#vcfg 1 || ! test -r $vcfg)
		vcfg=$arena0
	venti -m $vmpc -c $vcfg
	sleep 10			# wait for venti to start serving
}
if not if (! test -e /env/venti)
	venti=tcp!96.78.174.33!17034	# local default: fs

#
# figure out which root fossil partition to use, if any (don't search AoE),
# and start the root fossil, which must serve /srv/boot.
#
if (test -e    /dev/fs/stand)
	fossil=/dev/fs/stand
if not if (test -e /dev/fs/fossil)
	fossil=/dev/fs/fossil
if not if (test -e /dev/sd[~e-h]?/fossil) {
	fossil=    /dev/sd[~e-h]?/fossil
	fossil=$fossil(1)
}
if not
	fossil=/dev/no-fossil
if (test -r $fossil) {
	echo -n root fossil on $fossil...
	fossil -m 10 -f $fossil
	sleep 3				# wait for fossil to start serving
}

#
# mount new root.
# factotum is now mounted in /mnt; keep it visible.
# newns() needs it, among others.
#
rootdir=/root
rootspec=main/active			# used by /lib/namespace
# it really needs to be /srv/boot, as /lib/namespace assumes it.
if (test -e /srv/boot)
	srv=boot
if not if (test -e /srv/fossil)
	srv=fossil
if not if (test -e /srv/fossil.stand)
	srv=fossil.stand
if not if (test -e /srv/fsmain)
	srv=fsmain
if not {
	srv=no-root
	echo cannot find a root in /srv:
	ls -l /srv
}
echo -n mount -cC /srv/$srv $rootdir...
	mount -cC /srv/$srv $rootdir
bind -a $rootdir /

if (test -d $rootdir/mnt)
	bind -ac $rootdir/mnt /mnt
mount -b /srv/factotum /mnt

#
# bind a standard /bin and run init, which will run cpurc,
# then give the local user a bootes console shell.
#
if (! test -d /$cputype) {
	echo /$cputype missing!
	exec ./rc -m/boot/rcmain -i
}
bind /$cputype/bin /bin
bind -a /rc/bin /bin
path=(/bin . /boot)
# cpurc will have to start other fossils
echo init, cpurc...
/$cputype/init -c

# init died: let the local user repair what he can.
exec ./rc -m/boot/rcmain -i
