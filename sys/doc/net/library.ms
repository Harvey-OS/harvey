.NH
Library routines
.PP
The details of making and receiving connections across a network
were described in the section on protocol devices.
While straightforward, the dance is complicated enough to
be tedious.
The routines described here are provided to relieve the
programmer of most of the tedium.
.NH 2
Connecting
.PP
Making a connection to a remote destination uses a single
library call,
.I dial (2).
.I Dial
returns a file descriptor for the open data file of the connection.
.P1
int  dial(char *dest, char *local, char *dir, int *cfdp)
.P2
.IP \fIdest\fP
is the symbolic name/address of the destination.
.IP \fIlocal\fP
is the local address.
Since most networks do not allow this to be specified, it is
almost always zero.
.IP \fIdir\fP
is a pointer to a buffer for the path name of the protocol directory
representing this connection.
.I Dial
fills this buffer if the pointer is non-zero.
.IP \fIcfdp\fP
is a pointer to a file descriptor for the control file of the connection.
If the pointer is non-zero,
.CW dial
opens the control file.
.LP
Most programs call
.I dial
with only a destination name and all the other arguments 0.
Using CS,
.I dial
translates the symbolic name into all possible destination addresses.
It then attempts connecting to each one in turn until one works.
By using the special name
.CW net
as a network,
the user requests the system to pick a network/protocol in common
with the destination host and for which the requested service is valid.
For example, assume that the system
.I research.att.com
has the Datakit address
.I nj/astro/research
and the IP addresses
.I 135.104.117.5
and
.I 129.11.4.1.
The call
.P1
fd = dial("net!research.att.com!login", 0, 0, 0, 0);
.P2
will try in succession to connect to
.I nj/astro/research!login
on the Datakit and both
.I 135.104.117.5!513
and
.I 129.11.4.1!513
across the Internet.
.PP
A programmer may also use addresses instead of symbolic names
to dial out.
For example, the destinations
.I tcp!135.104.117.5!513
and
.I tcp!research.att.com!login
are both valid arguments to
.I dial
for the same machine.
Since we use unicode representations everywhere,
no commands or library routines need make
a distinction between the use of addresses
and names.
.NH 2
Listening
.PP
Four routines are used to listen for incoming connections.
A program first
.I announce (2)'s
its intention to receive connections.
It then
.I listen (2)'s
for calls and finally
.I accept (2)'s
or
.I reject (2)'s
them.
.I Announce
returns a file descriptor for the control file and the
path name of the protocol directory (in
.I dir )
for the announcement.
.P1
int  announce(char *addr, char *dir)
.P2
.I Addr
is the symbolic name/address announced.
If it does not contain a service, the announcement is for
all services not explicitly announced.
Thus, one can easily write the equivalent of the
.CW inetd
program to listen for incoming requests without
having to announce each separate service.
The announcement remains in force until the control file is
closed.
.LP
.I Listen
returns a file descriptor for the control file and the path
name of the protocol directory (in
.I ldir )
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
from the call to
.I listen.
Some networks such as Datakit allow the reason for the rejection.
Networks such as IP ignore the argument.
.P1
int  accept(int ctl, char *ldir)
int  reject(int ctl, char *ldir, char *reason)
.P2
.PP
The following is the code for a prototype TCP listener.
The code announces itself, listens for connections and forks a new
process for each connection.
The new process echoes data on the connection until the
remote end closes it.
The "*" in the symbolic name means the announcement is valid for
any addresses bound to the machine the program is run on.
.P1
int
echo_server(void)
{
	int dfd, acfd, lcfd;
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
