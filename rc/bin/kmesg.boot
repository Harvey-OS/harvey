#!/bin/rc
# gather /dev/kmesg contents since boot into /sys/log/kmesg

cd /sys/log/kmesg
cp /dev/kmesg $sysname.temp
{
	echo; echo; date
	if (grep -s '^Plan 9($| )' $sysname.temp) {
		echo '?^Plan 9($| )?,$p' | ed - $sysname.temp
	}
	if not
		tail -50 $sysname.temp
	echo
	cat /dev/swap
} >>$sysname
rm -f $sysname.temp
