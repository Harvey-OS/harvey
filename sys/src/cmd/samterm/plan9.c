#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

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
	if (n == 0)
		*tosam = 0;
	else
		s1 = realloc(s1, n);
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
