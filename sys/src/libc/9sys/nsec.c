/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <tos.h>

static uint64_t order = 0x0001020304050607ULL;

static void
be2vlong(int64_t* to, uint8_t* f)
{
	uint8_t* t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&order;
	for(i = 0; i < sizeof order; i++)
		t[o[i]] = f[i];
}

static int fd = -1;
static struct {
	int pid;
	int fd;
} fds[64];

int64_t
nsec(void)
{
	uint8_t b[8];
	int64_t t;
	int pid, i, f, tries;

	/*
	 * Threaded programs may have multiple procs
	 * with different fd tables, so we may need to open
	 * /dev/bintime on a per-pid basis
	 */

	/* First, look if we've opened it for this particular pid */
	if((pid = _tos->pid) == 0) /* 9vx bug, perhaps? */
		_tos->pid = pid = getpid();
	do {
		f = -1;
		for(i = 0; i < nelem(fds); i++)
			if(fds[i].pid == pid) {
				f = fds[i].fd;
				break;
			}
		tries = 0;
		if(f < 0) {
			/* If it's not open for this pid, try the global pid */
			if(fd >= 0)
				f = fd;
			else {
				/* must open */
				if((f = open("/dev/bintime", OREAD | OCEXEC)) <
				   0)
					return 0;
				fd = f;
				for(i = 0; i < nelem(fds); i++)
					if(fds[i].pid == pid ||
					   fds[i].pid == 0) {
						fds[i].pid = pid;
						fds[i].fd = f;
						break;
					}
			}
		}
		if(pread(f, b, sizeof b, 0) == sizeof b) {
			be2vlong(&t, b);
			return t;
		}
		close(f);
		if(i < nelem(fds))
			fds[i].fd = -1;
	} while(tries++ == 0); /* retry once */
	USED(tries);
	return 0;
}
