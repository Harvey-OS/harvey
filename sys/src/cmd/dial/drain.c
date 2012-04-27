#include <u.h>
#include <libc.h>

void
ding(void*, char *s)
{
	if(strstr(s, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

void
main(void)
{
	char buf[256];

	alarm(100);
	while(read(0, buf, sizeof(buf)) > 0)
		;
	alarm(0);
	exits(0);
}
