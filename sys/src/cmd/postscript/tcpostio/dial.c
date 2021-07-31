#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#define	DIALTIMEOUT	30

/* This is a dummy routine for non Plan9 systems.
 * No attempt has been made to be clever, it's just
 * supposed to work in this program.
 */
int dial_debug = 0;

int
dial(char *dest, char *local, char *dir, int *cfdp) {
	int sockconn, lport;
	struct hostent *hp;		/* Pointer to host info */
	struct sockaddr_in sin;		/* Socket address, Internet style */
	struct servent *sp = 0;
	char *tdest, *netname, *hostname, *servname;
	int sock_type;
#ifndef plan9
#define	USED(x)	if(x); else
	int sockoption, sockoptsize;
#endif

	USED(dir);
	USED(cfdp);
	if ((tdest = malloc(strlen(dest)+1)) == NULL) {
		if (dial_debug) fprintf(stderr, "dial: could not allocate memory\n");
		return(-1);
	}
	strcpy(tdest, dest);

	if ((netname = strtok(tdest, "!")) == NULL) {
		fprintf(stderr, "dial: no network name\n");
		return(-1);
	}
	if (strcmp(netname, "tcp") == 0) {
		sock_type = SOCK_STREAM;
	} else if (strcmp(netname, "udp") == 0) {
		sock_type = SOCK_DGRAM;
	} else {
		fprintf(stderr, "dial: network protocol name `%s' is invalid; must be `tcp' or `udp'\n", netname);
		return(-1);
	}
	if ((hostname = strtok(0, "!")) == NULL) {
		fprintf(stderr, "dial: no host name or number\n");
		return(-1);
	}
	if ((servname = strtok(0, "!")) == NULL) {
		fprintf(stderr, "dial: no service name or number\n");
		return(-1);
	}
	hp = gethostbyname(hostname);
	if (hp == (struct hostent *)NULL) {
		if (dial_debug) fprintf(stderr, "host `%s' unknown by local host\n", hostname);
		return(-1);
	}
	if (!isdigit(servname[0]))
		sp = getservbyname(servname, netname);
	sin.sin_addr.s_addr = *(unsigned long*)hp->h_addr;
	sin.sin_port	= htons((sp==0)?atoi(servname):sp->s_port);
	sin.sin_family	= AF_INET;
	if (local == NULL) {
		if ((sockconn = socket(AF_INET, sock_type, 0)) < 0) {
			if (dial_debug) perror("dial:socket():");
			return(-1);
		}
		if (dial_debug) fprintf(stderr, "socket FD=%d\n", sockconn);
	} else {
		lport = atoi(local);
		if ((lport < 512) || (lport >= 1024)) {
			fprintf(stderr, "dial:invalid local port %d\n", lport);
			return(-1);
		}
		if ((sockconn = rresvport(&lport)) < 0) {
			if (dial_debug) perror("dial:rresvport():");
			return(-1);
		}
	}
	if (dial_debug) {
		fprintf(stderr, "sin size=%d\n", sizeof(sin));
	}
	alarm(DIALTIMEOUT);
	if ((connect(sockconn, (struct sockaddr *) &sin, sizeof(sin)) < 0)) {
		if (dial_debug) perror("dial:connect():");
		return(-1);
	}
	alarm(0);
#ifndef plan9
	sockoptsize = sizeof(sockoption);
	if (getsockopt(sockconn, SOL_SOCKET, SO_KEEPALIVE, &sockoption, &sockoptsize) < 0) {
		if (dial_debug) perror("dial:getsockopt():");
		return(-1);
	}
	if (sockoptsize == sizeof(sockoption) && !sockoption) {
		if (setsockopt(sockconn, SOL_SOCKET, SO_KEEPALIVE, &sockoption, sockoptsize) < 0) {
			if (dial_debug) perror("dial:getsockopt():");
			return(-1);
		}
	}
#endif
	return(sockconn);
}
