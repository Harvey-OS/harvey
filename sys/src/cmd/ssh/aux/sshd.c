#include <u.h>
#include <libc.h>
#include <bio.h>	/* for ndb.h */
#include <ndb.h>

#include "../ssh.h"

#define SERVER "./8.sshserve"	/* Full path name */

char *progname;

char		client_host_ip[32];

int dfd;	/* File descriptors for TCP communications */

static int	getclientip(char *);

void
main(int argc, char *argv[]) {
	int cfd;
	char adir[40], ldir[40];

	USED(argc);
	progname = argv[0];
	fprint(2, "starting %d\n", DBG);
	debug(DBG, "Starting\n");
	/* open port to listen -- TCP port 22 (or 333 for debugging) --
	 * for new connections
	 */
	if (announce("tcp!*!333", adir) < 0) error("announce: %r\n");
	debug(DBG, "Announcing\n");

	/* when new connection request arrives, fork off child
	 * to deal with it
	 */
	for(;;){
	    /* listen for a call */
	    cfd = listen(adir, ldir);
	    if(cfd < 0) error("listen: %r");
	    debug(DBG, "Call announced\n");
	    /* fork a servlet */
	    switch(fork()){
	    case -1:
		fprint(2, "%s: fork: %r\n", progname);
		close(cfd);
		break;
	    case 0:
		/* accept the call and open the data file */
		dfd = accept(cfd, ldir);
		if(dfd < 0) error("accept: %r");
		if (getclientip(ldir)) {
			debug(DBG, "Call accepted from %s\n",
				client_host_ip);
			dup(dfd, 0);
			dup(dfd, 1);
			execl(SERVER, "sshserve", client_host_ip, 0);
			error("Can't exec %s", SERVER);
		}
		exits(0);
	    default:
		close(cfd);
		break;
	    }
	}
}

static int
getclientip(char *tcpdir) {
	int fd, n;
	char fn[128];

	snprint(fn, sizeof(fn), "%s/%s", tcpdir, "remote");
	if ((fd = open(fn, OREAD)) < 0)
		error("Can't open %s", fn);
	if ((n = read(fd, client_host_ip, 31)) <= 0)
		error("Can't read %s", fn);
	client_host_ip[n] = 0;
	debug(DBG_IO, "Remote ip is `%s'\n", client_host_ip);

	return 1;
}
