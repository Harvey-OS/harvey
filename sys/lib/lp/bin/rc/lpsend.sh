#!/bin/rc
bind -b /sys/lib/lp/bin/$cputype /bin
if (! ~ $DEBUG '') { flag x + }
# tcp file transfer is broken on plan 9. philw thinks its a flow control problem
# with plan 9 streams.
# if (test -e /net/tcp/clone || import helix /net/tcp) {
# 	dialstring=`{sed -n '/name=tcp!/s/name=tcp!\([^`]*\).*/\1'$1'/p' /rc/bin/m/$1}
# 	network=tcp
# 	if (! ~ $dialstring '') {
# 		if(lpsend $dialstring $network printer) exit ''
# 		rv='tcp failed'
# 	}
# }
# if not rv='no tcp'

# try not to use tcpgate for a while. 9201021250
# tcp file transfer still broken. 9201171404
dialstring=`{sed -n '/name=tcp!/s/name=tcp!\([^`]*\).*/\1'$1'/p' /rc/bin/m/$1}
network=tcpgate
if (! ~ $dialstring '') {
	if(lpsend $dialstring $network printer) exit ''
	rv='tcpgate failed'
}

if (test -e /net/dk/clone || import helix /net/dk) {
 	dialstring=`{sed -n '/name=dk!/s/name=dk!\([^`]*\).*/\1'$1'/p' /rc/bin/m/$1}
	network=dk
	if (! ~ $dialstring '') {
		if(lpsend $dialstring $network printer) exit ''
	}
	rv=$rv^', dk failed'
}
if not rv=$rv^', no dk'

if (! ~ $dialstring '')
	exit 'lpsend: no dialstring'
if not
	exit 'lpsend: '^$rv
