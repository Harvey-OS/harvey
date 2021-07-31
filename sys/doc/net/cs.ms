.NH 2
Network Database
.PP
On most systems several
files such as
.CW /etc/hosts ,
.CW /etc/networks ,
.CW /etc/services ,
.CW /etc/hosts.equiv ,
.CW /etc/bootptab ,
and
.CW /etc/named.d
hold network information.
Much time and effort is spent
administering these files and keeping
them mutually consistent.
Tools attempt to
automatically derive one or more of the files from
information in other files but maintenance continues to be
difficult and error prone.
.PP
Since we wrote our world from scratch, we decided to
avoid this nightmare.
One database on a shared server contains all the information
needed for network administration.
Two ASCII files comprise the main database:
.CW /lib/ndb/local
contains locally administered information and
.CW /lib/ndb/global
contains information imported from elsewhere.
The files contain sets of attribute/value pairs of the form
.I attr=value ,
where
.I attr
and
.I value
are alphanumeric strings.
Systems are described by multi-line entries;
a header line at the left margin begins each entry followed by zero or more
indented attribute/value pairs specifying
names, addresses, properties, etc.
For example, the entry for our CPU server
specifies a domain name, an IP address, an Ethernet address,
a Datakit address, a boot file, and supported protocols.
.P1
sys = helix
	dom=helix.research.att.com
	bootf=/mips/9power
	ip=135.104.9.31 ether=0800690222f0
	dk=nj/astro/helix
	proto=il flavor=9cpu
.P2
If several systems share entries such as
network mask and gateway, we specify that information
with the network or subnetwork instead of the system.
The following entries define a class B IP network and 
a few subnets derived from it.
The entry for the network specifies the IP mask,
file system, and authentication server for all systems
on the network.
Each subnetwork specifies its default IP gateway.
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
Database entries also define the mapping of service names
to port numbers for TCP, UDP, and IL.
.P1
tcp=echo	port=7
tcp=discard	port=9
tcp=systat	port=11
tcp=daytime	port=13
.P2
.PP
All programs read the database directly so
there are no intermediate files or binary format and
consistency problems are rare.
However the database files can become large.
Our global file, containing all information about
both Datakit and Internet systems in AT&T, has 43,000
lines.
To speed searches, we build hash table files for each
attribute we expect to search often.
The hash file entries point to entries
in the master files.
Every hash file contains the modification time of its master
file so we can avoid using an out-of-date hash table.
Searches for attributes that aren't hashed or whose hash table
is out-of-date still work, they just take longer.
.NH 2
Connection Server
.PP
On each system a user level connection server process, CS, performs
symbolic name to address translation.
CS uses information about available networks, the network database, and
other servers (such as DNS) to translate names.
CS is a file server serving a single file,
.CW /net/cs .
A client writes a symbolic name to
.CW /net/cs
then reads one line for each matching destination reachable
from this system.
The lines are of the form
.I "filename message",
where
.I filename
is the path of the clone file to open for a new connection and
.I message
is the string to write to it to make the connection.
The following example illustrates this.
.CW Ndb/csquery
is a program that prompts for strings to write to
/net/cs and prints the replies.
.P1
% ndb/csquery
> net!helix!9fs
/net/il/clone 135.104.9.31!17008
/net/dk/clone nj/astro/helix!9fs
.P2
.PP
CS provides meta-name translation to perform complicated
searches.
The special network name
.CW net
selects any network in common between source and
destination supporting the specified service.
A host name of the form
.I $attr
is the name of an attribute in the network database.
The database search returns the value
of the matching attribute/value pair
most closely associated with the source host.
Most closely associated is defined on a per network basis.
For example, the symbolic name
.I tcp!$auth!rexauth
causes CS to search for the
.I auth
attribute in the database entry for the source system, then its
subnetwork (if there is one) and then its network.
.P1
% ndb/csquery
> net!$auth!rexauth
/net/il/clone 135.104.9.34!17021
/net/dk/clone nj/astro/p9auth!rexauth
/net/il/clone 135.104.9.6!17021
/net/dk/clone nj/astro/musca!rexauth
.P2
.PP
Normally CS derives naming information from its database files.
For domain names however, CS first consults another user level
process, the domain name server (DNS).
If no DNS is reachable, CS relies on its own tables.
.PP
Like CS, the domain name server is a user level process providing
one file,
.CW /net/dns .
A client writes a request of the form
.I "domain-name type" ,
where
.I type
is a domain name service resource record type.
DNS performs a recursive query through the
Internet domain name system producing one line
per resource record found.  The client reads
.CW /net/dns 
to retrieve the records.
Like other domain name servers, DNS caches information
learned from the network.
DNS is implemented as a multi-process shared memory application
with separate processes listening for network and local requests.
