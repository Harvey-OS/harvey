#include <u.h>
#include <libc.h>

/*  console state (for consctl) */
typedef struct Consstate	Consstate;
struct Consstate{
	int raw;
	int hold;
};

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

	if(bind("#|", "/mnt/term", MBEFORE) < 0
	|| bind("/mnt/term/data1", "/dev/consctl", MREPL) < 0)
		return 0;

	switch(fork()){
	case -1:
		return 0;
	case 0:
		break;
	default:
		return x;
	}

	setfields(" ");
	for(tries = 0; tries < 100; tries++){
		x->raw = 0;
		x->hold = 0;
		fd = open("/mnt/term/data", OREAD);
		if(fd < 0)
			continue;
		tries = 0;
		for(;;){
			n = read(fd, buf, sizeof(buf)-1);
			if(n <= 0)
				break;
			buf[n] = 0;
			n = getmfields(buf, field, 10);
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
}
