#!/bin/sh

trap "" 1 2

cd $LPSPOOL/queue

POSTIOLOG="$LPSPOOL/log/${LPDEST}.st"
MAILLOG=/usr/spool/mail/mail.log
export MAILLOG POSTIOLOG
case "$THIS_HOST" in
"$DEST_HOST")
	( trap "exit 0" 15
	while true
	do
		error=`tail -2 $POSTIOLOG | grep PrinterError`
		case "$error" in
		"")	sleep 40
			;;
		*)
			set "" `cat $LPSPOOL/queue/$LPDEST/*id | sort -u`; shift
			while [ "$1" != "" ]
			do
				echo "delivered $2 From nopaper (vwho)" >> /n/$1/$MAILLOG
				shift 2
			done
			sleep 200	# give someone a chance to fix it
			;;
		esac
	done ) &
	WARN=$!
	;;
esac

generic 'eval /usr/bin/postscript/postio -R2 -B4096 -l$OUTDEV -b$SPEED -L$PRINTLOG $LPDEST/$FILE' 'eval echo -d"$LPDEST" -pnoproc -M"$1" -u"$2" | cat - $LPDEST/$FILE | lpsend "$DEST_HOST"' 'eval grep -l $2 $LPDEST/*id | grep -v $LPDEST/${FILE}id > /dev/null 2>&1; case $? in 0) echo "delivered $2 From applemore (vwho)" >> /n/$1/$MAILLOG;; *) echo "delivered $2 From appledone (vwho)" >> /n/$1/$MAILLOG;esac'


case "$THIS_HOST" in
"$DEST_HOST")
	kill -9 $WARN;;
esac
