#include <u.h>
#include <libc.h>
#include <draw.h>
#include "cons.h"

/*
 *  create a shared segment.  Make is start 2 meg higher than the current
 *  end of process memory.
 */
static void*
share(int len)
{
	ulong vastart;

	vastart = ((ulong)sbrk(0)) + 2*1024*1024;

	if(segattach(0, "shared", (void *)vastart, len) < 0)
		return 0;
	memset((void*)vastart, 0, len);

	return (void*)vastart;
}

/*
 *  bind a pipe onto consctl and keep reading it to
 *  get changes to console state.
 */
Consstate*
consctl(void)
{
	int i, n;
	int fd;
	int tries;
	char buf[128];
	Consstate *x;
	char *field[10];

	x = share(sizeof(Consstate));
	if(x == 0)
		return 0;

	/* a pipe to simulate consctl */
	if(bind("#|", "/mnt/cons/consctl", MBEFORE) < 0
	|| bind("/mnt/cons/consctl/data1", "/dev/consctl", MREPL) < 0){
		fprint(2, "error simulating consctl\n");
		exits("/dev/consctl");
	}

	/* a pipe to simulate the /dev/cons */
	if(bind("#|", "/mnt/cons/cons", MREPL) < 0
	|| bind("/mnt/cons/cons/data1", "/dev/cons", MREPL) < 0){
		fprint(2, "error simulating cons\n");
		exits("/dev/cons");
	}

	switch(fork()){
	case -1:
		return 0;
	case 0:
		break;
	default:
		return x;
	}

	notify(0);

	for(tries = 0; tries < 100; tries++){
		x->raw = 0;
		x->hold = 0;
		fd = open("/mnt/cons/consctl/data", OREAD);
		if(fd < 0)
			break;
		tries = 0;
		for(;;){
			n = read(fd, buf, sizeof(buf)-1);
			if(n <= 0)
				break;
			buf[n] = 0;
			n = getfields(buf, field, 10, 1, " ");
			for(i = 0; i < n; i++){
				if(strcmp(field[i], "rawon") == 0)
					x->raw = 1;
				else if(strcmp(field[i], "rawoff") == 0)
					x->raw = 0;
				else if(strcmp(field[i], "holdon") == 0)
					x->hold = 1;
				else if(strcmp(field[i], "holdoff") == 0)
					x->hold = 0;
			}
		}
		close(fd);
	}
	exits(0);
	return 0;		/* dummy to keep compiler quiet*/
}
