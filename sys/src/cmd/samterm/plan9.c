#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

static char exname[64];

void
getscreen(int argc, char **argv)
{
	USED(argc);
	USED(argv);
	binit(panic, 0, "sam");
	bitblt(&screen, screen.clipr.min, &screen, screen.clipr, 0);
}

int
snarfswap(char *fromsam, int nc, char **tosam)
{
	char *s1;
	int f, n;

	f = open("/dev/snarf", 0);
	if(f < 0)
		return -1;
	*tosam = s1 = alloc(SNARFSIZE);
	n = read(f, s1, SNARFSIZE-1);
	close(f);
	if(n < 0)
		n = 0;
	if (n == 0) {
		*tosam = 0;
		free(s1);
	} else
		s1[n] = 0;
	f = create("/dev/snarf", 1, 0666);
	if(f >= 0){
		write(f, fromsam, nc);
		close(f);
	}
	return n;
}

void
dumperrmsg(int count, int type, int count0, int c)
{
	fprint(2, "samterm: host mesg: count %d %ux %ux %ux %s...ignored\n",
		count, type, count0, c, rcvstring());
}

void
removeextern(void)
{
	remove(exname);
}

void
extstart(void)
{
	char buf[32];
	int fd, p[2];

	if(pipe(p) < 0)
		return;
	sprint(exname, "/srv/sam.%s", getuser());
	fd = create(exname, 1, 0600);
	if(fd < 0){
		remove(exname);
		fd = create(exname, 1, 0600);
		if(fd < 0){
	Err:
			close(p[0]);
			close(p[1]);
			return;
		}
	}
	sprint(buf, "%d", p[0]);
	if(write(fd, buf, strlen(buf)) <= 0)
		goto Err;
	close(fd);
	/*
	 * leave p[0] open so if the file is removed the event
	 * library won't get an error
	 */
	estart(Eextern, p[1], 8192);
	atexit(removeextern);
}
