#include <u.h>
#include <libc.h>
#include <tos.h>

static uvlong order = 0x0001020304050607ULL;

static void
be2vlong(vlong *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&order;
	for(i = 0; i < sizeof order; i++)
		t[o[i]] = f[i];
}

static int fd = -1;
static struct {
	int	pid;
	int	fd;
} fds[64];

vlong
nsec(void)
{
	uchar b[8];
	vlong t;
	int pid, i, f, tries;

	/*
	 * Threaded programs may have multiple procs
	 * with different fd tables, so we may need to open
	 * /dev/bintime on a per-pid basis
	 */

	/* First, look if we've opened it for this particular pid */
	pid = _tos->pid;
	do{
		f = -1;
		for(i = 0; i < nelem(fds); i++)
			if(fds[i].pid == pid){
				f = fds[i].fd;
				break;
			}
		tries = 0;
		if(f < 0){
			/* If it's not open for this pid, try the global pid */
			if(fd >= 0)
				f = fd;
			else{
				/* must open */
				if((f = open("/dev/bintime", OREAD|OCEXEC)) < 0)
					return 0;
				fd = f;
				for(i = 0; i < nelem(fds); i++)
					if(fds[i].pid == pid || fds[i].pid == 0){
						fds[i].pid = pid;
						fds[i].fd = f;
						break;
					}
			}
		}
		if(pread(f, b, sizeof b, 0) == sizeof b){
			be2vlong(&t, b);
			return t;
		}
		close(f);
		if(i < nelem(fds))
			fds[i].fd = -1;
	}while(tries++ == 0);	/* retry once */
	USED(tries);
	return 0;
}
