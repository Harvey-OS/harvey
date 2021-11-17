j(w1.6.0-w1.2.0)(l0-l474) touch superblock 35709856
emelie: version
31-bit emelie as of Mon Feb 12 19:28:40 2007
	last boot Mon Feb 12 19:29:31 2007
emelie: stats
cons stats
	work =      0      0      0 rps
	rate =      0      0      0 tBps
	hits =      0      0      0 iops
	read =      0      0      0 iops
	rah  =      0      0      0 iops
	init =      0      0      0 iops
	bufs =    500 sm 100 lg 0 res
	ioerr=      0 wr   0 ww   0 dr   0 dw
	cache=             0 hit         0 miss
emelie: printconf
config w0
service emelie
filsys main c[w<0-5>]j(w1.<6-2>.0)(l<0-236>l<238-474>)
filsys dump o
filsys other [w<9-14>]
filsys temp w8
ipauth 135.104.9.7
ipsntp 135.104.9.52
ip0 135.104.9.42
ipgw0 135.104.9.1
ipmask0 255.255.255.0
end
emelie: 
