.NH
Library routines
.PP
The section on protocol devices described the details
of making and receiving connections across a network.
The dance is straightforward but tedious.
Library routines are provided to relieve
the programmer of the details.
.NH 2
Connecting
.PP
The
.I dial (2)
library call establishes a connection to a remote destination.
It
returns an open file descriptor for the
.CW data
file in the connection directory.
.P1
int  dial(char *dest, char *local, char *dir, int *cfdp)
.P2
.IP \fIdest\fP
is the symbolic name/address of the destination.
.IP \fIlocal\fP
is the local address.
Since most networks do not support this, it is
almost always zero.
.IP \fIdir\fP
is a pointer to a buffer to hold the path name of the protocol directory
representing this connection.
.I Dial
fills this buffer if the pointer is non-zero.
.IP \fIcfdp\fP
is a pointer to a file descriptor for the
.CW ctl
file of the connection.
If the pointer is non-zero,
.CW dial
opens the control file and tucks the file descriptor here.
.LP
Most programs call
.I dial
with a destination name and all other arguments zero.
.I Dial
uses CS to
translate the symbolic name to all possible destination addresses
and attempts to connect to each in turn until one works.
Specifying the special name
.CW net
in the network portion of the destination
allows CS to pick a network/protocol in common
with the destination for which the requested service is valid.
For example, assume the system
.I research.att.com
has the Datakit address
.I nj/astro/research
and IP addresses
.I 135.104.117.5
and
.I 129.11.4.1.
The call
.P1
fd = dial("net!research.att.com!login", 0, 0, 0, 0);
.P2
tries in succession to connect to
.I nj/astro/research!login
on the Datakit and both
.I 135.104.117.5!513
and
.I 129.11.4.1!513
across the Internet.
.PP
.I Dial
accepts addresses instead of symbolic names.
For example, the destinations
.I tcp!135.104.117.5!513
and
.I tcp!research.att.com!login
are equivalent
references to the same machine.
.NH 2
Listening
.PP
A program uses
four routines to listen for incoming connections.
It first
.I announce (2)'s
its intention to receive connections,
then
.I listen (2)'s
for calls and finally
.I accept (2)'s
or
.I reject (2)'s
them.
.I Announce
returns an open file descriptor for the
.CW ctl
file of a connection and fills
.I dir
with the
path of the protocol directory
for the announcement.
.P1
int  announce(char *addr, char *dir)
.P2
.I Addr
is the symbolic name/address announced;
if it does not contain a service, the announcement is for
all services not explicitly announced.
Thus, one can easily write the equivalent of the
.CW inetd
program without
having to announce each separate service.
An announcement remains in force until the control file is
closed.
.LP
.I Listen
returns an open file descriptor for the
.CW ctl
file and fills
.I ldir
with the path
of the protocol directory
for the received connection.
It is passed
.I dir
from the announcement.
.P1
int  listen(char *dir, char *ldir)
.P2
.LP
.I Accept
and
.I reject
are called with the control file descriptor and
.I ldir
returned by
.I listen.
Some networks such as Datakit accept a reason for a rejection;
networks such as IP ignore the third argument.
.P1
int  accept(int ctl, char *ldir)
int  reject(int ctl, char *ldir, char *reason)
.P2
.PP
The following code implements a typical TCP listener.
It announces itself, listens for connections and forks a new
process for each.
The new process echoes data on the connection until the
remote end closes it.
The "*" in the symbolic name means the announcement is valid for
any addresses bound to the machine the program is run on.
.P1
int
echo_server(void)
{
	int dfd, lcfd;
	char adir[40], ldir[40];
	int n;
	char buf[256];

	afd = announce("tcp!*!echo", adir);
	if(afd < 0)
		return -1;

	for(;;){
		/* listen for a call */
		lcfd = listen(adir, ldir);
		if(lcfd < 0)
			return -1;

		/* fork a process to echo */
		switch(fork()){
		case 0:
			/* accept the call and open the data file */
			dfd = accept(lcfd, ldir);
			if(dfd < 0)
				return -1;

			/* echo until EOF */
			while((n = read(dfd, buf, sizeof(buf))) > 0)
				write(dfd, buf, n);
			exits(0);
		case -1:
			perror("forking");
		default:
			close(lcfd);
			break;
		}

	}
}
.P2
