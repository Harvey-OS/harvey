#!/bin/rc
tf=/tmp/xbm2pic.$pid
cat $1 >$tf
awk '/^#define.*width/{wid=$3+7-($3+7)%8}
     /^#define.*height/{hgt=$3}
     /^static/{printf("TYPE=dump\nWINDOW=0 0 %d %d\nNCHAN=1\nCHAN=m\n\n", wid, hgt);}' $tf
grep 0x $tf|sed 's/0x//g
		s/[^0-9a-fA-F]//g
		s/(.)(.)/\2\1/g
		s/0/..../g
		s/1/x.../g
		s/2/.x../g
		s/3/xx../g
		s/4/..x./g
		s/5/x.x./g
		s/6/.xx./g
		s/7/xxx./g
		s/8/...x/g
		s/9/x..x/g
		s/[aA]/.x.x/g
		s/[bB]/xx.x/g
		s/[cC]/..xx/g
		s/[dD]/x.xx/g
		s/[eE]/.xxx/g
		s/[fF]/xxxx/g'|tr -d '\012'|tr .x '\377\000'|tcs -t latin1
rm -f $tf
