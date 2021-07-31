#!/bin/rc
if(! test -f /srv/dos)
	dossrv >/dev/null </dev/null >[2]/dev/null
unmount /n/a:>[2]/dev/null

if(~ $#adisk 1)
	;	# do nothing
if not if(~ $#bootfile 0)
	adisk=/dev/fd0disk
if not {
	switch($bootfile) {
	case sd*
		adisk=`{echo $bootfile | sed 's#(sd..).*#/dev/\1/data#'}
	case fd*
		adisk=`{echo $bootfile | sed 's#(fd.).*#/dev/\1disk#'}
	case *
		echo 'unknown bootfile '^$bootfile^'; mail 9trouble@plan9.bell-labs.com'
		exit oops
	}
}

mount -c /srv/dos /n/a: $adisk
