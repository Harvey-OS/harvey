#include <u.h>
#include <libc.h>

static int
ding(void *x, char *msg)
{
	USED(x);
	print("%s\n", msg);
	return strstr(msg, "alarm")!=0;
}

void
main(void)
{
	int pfd[2];
	char buf[2];

	pipe(pfd);
	for(;;){
		atnotify(ding, 1);
		alarm(1000);
		read(pfd[0], buf, 2);
		alarm(0);
		atnotify(ding, 0);
	}
}
