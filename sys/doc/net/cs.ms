.NH 2
Network Database
.PP
Network information on most systems is held in a number
of files such as
.CW /etc/hosts ,
.CW /etc/networks ,
.CW /etc/services ,
.CW /etc/hosts.equiv ,
.CW /etc/bootptab ,
.CW /etc/named.d ,
etc.
A major component of administering a computing
environment is maintaining these files and keeping
them mutually consistent.
This has been aided by a number of tools that try to
automatically derive one or more of the files from
information in other files.
Despite such aids maintenance continues to be
difficult and error prone.
The only real reason for this plethora of files is that
each appeared as an adjunct to some new tool.
.PP
Since we wrote our world from scratch, we decided to
avoid this nightmare.
One database on a shared server contains all the information
needed for network administration.
The main database is composed of two ascii files,
.CW /lib/ndb/local
containing information locally administered and
.CW /lib/ndb/global
containing information imported from elsewhere.
The files contain sets of attribute/value pairs of the form
.I attr=value .
An unindented line starts a new set.
Both attribute and value can be any alphanumeric string.
Systems are described by multi-line entries giving
names, addresses, properties, etc.
For example, the set for our main CPU server
specifies a domain name, an IP address, an Ethernet address,
a Datakit address, a boot file, and protocols supported.
.P1
sys = helix
	dom=helix.research.att.com
	bootf=/mips/9power
	ip=135.104.9.31 ether=0800690222f0
	dk=nj/astro/helix
	proto=il flavor=9cpu
.P2
If there are many systems sharing the same entries such as
network mask and gateway, we specify that information
with the network or subnetwork instead of the system.
The following entries define a class B IP network and 
a few subnets derived from it.
The entry for the network specifies the ipmask,
file system, and authentication server to be used by all systems
on the network.
Each subnetwork specifies the default IP gateway for that
subnetwork.
.P1
ipnet=mh-astro-net ip=135.104.0.0 ipmask=255.255.255.0
	fs=bootes.research.att.com
	auth=1127auth
ipnet=unix-room ip=135.104.117.0
	ipgw=135.104.117.1
ipnet=third-floor ip=135.104.51.0
	ipgw=135.104.51.1
ipnet=fourth-floor ip=135.104.52.0
	ipgw=135.104.52.1
.P2
The mapping of service names to port numbers for TCP, UDP, and
IL is also contained in the database.
.P1
tcp=echo	port=7
tcp=discard	port=9
tcp=systat	port=11
tcp=daytime	port=13
.P2
.PP
All programs using the database read it directly.
There are no intermediate files or binary format.
Therefore, consistency problems are rare.
However the database files can become quite large.
Our global file, which contains all information about
both Datakit and Internet systems in AT&T, is 43000
lines long.
To speed up searches, we build hash table files for each
attribute that we expect to search a lot.
The hash files contain pointers to entries
in the master files.
Since hash files contain the modify time of the master
file they pertain to, we never use out of date hash tables.
Searches for attributes that aren't hashed or whose hash table
is out of date still work, they just take longer.
.NH 2
Connection Server
.PP
Each system runs a user level connection server process, CS, to perform
symbolic name to address translation.
CS uses information about available networks, the network database, and
other servers (such as DNS) to translate names.
As we stated earlier, CS is a file server that presents one file
.CW /net/cs .
For any symbolic name written to
.CW /net/cs
CS responds with one line for each matching destination that might be reachable
from this system.
The lines are of the form
.I "filename message",
where
.I filename
is the clone file to open for a new connection and
.I message
is what to write into it to make the connection.
.PP
CS provides meta-name translation to perform more complicated
searches.
The special network name
.CW net
is a meta-name standing for any network in common between source and
destination that supports the specified service.
A host name of the form
.I $attr
is a meta-name standing for a search in the network database.
The translation comes from the value of the matching attribute/value pair
most closely associated with the source host.
Most closely associated is defined on a per network basis.
For example, for the symbolic name
.I tcp!$auth!rexauth
CS would search for first in the entry for the source system, then its
subnetwork (if there was one) and then its network.
.PP
Normally, CS derives naming information from its database files.
For domain names, however, CS first consults another user level
process, the domain name server (DNS).
If no DNS is reachable, CS will then search its own tables.
.PP
Like CS, the domain name server is a user level process providing
one file,
.CW /net/dns .
A client writes a request into the file of the form
.I "domain-name type" ,
where
.I type
is a domain name service resource record type.
DNS will then perform a recursive query through the
Internet domain name system and return one line
per resource record found.
Like other domain name servers, DNS caches information
learned from the network.
DNS also has a UDP interface to answer requests from
other systems.
It runs as a multi-process shared memory application.
