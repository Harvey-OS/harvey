CLOG=lookout
LOG=auth cs dns fax ipboot listen mail telco runq cron timesync smtp ssh

all:V:
	cd /sys/log
	day=`{date|sed 's/(^[^ ]*) .*/\1/'}
	for(i in $LOG){
		if(test -e $i){
			cp $i $i.$day
			chmod 664 $i.$day
		}
		rm -f $i
		> $i
		chmod +arw $i
	}
	for(i in $CLOG){
		if(test -e $i){
			cp $i $i.$day
			chmod 664 $i.$day
		}
		rm -f $i
	}

startclog:VQ:
	for(i in $CLOG){
		aux/clog /mnt/consoles/$i /sys/log/$i &
	}
