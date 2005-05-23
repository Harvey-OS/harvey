#include <u.h>
#include <libc.h>

/*
 * like fprint but be sure to deliver as a single write.
 * (fprint uses a small write buffer.)
 */
void
xfprint(int fd, char *fmt, ...)
{
	char *s;
	va_list arg;

	va_start(arg, fmt);
	s = vsmprint(fmt, arg);
	va_end(arg);
	if(s == nil)
		sysfatal("smprint: %r");
	write(fd, s, strlen(s));
	free(s);
}

void
main(int argc, char **argv)
{
	int fd;
	char dir[512];

	fd = open("/dev/acme/ctl", OWRITE);
	if(fd < 0)
		exits(0);
	getwd(dir, 512);
	if(dir[0]!=0 && dir[strlen(dir)-1]=='/')
		dir[strlen(dir)-1] = 0;
	xfprint(fd, "name %s/-%s\n",  dir, argc > 1 ? argv[1] : "rc");
	xfprint(fd, "dumpdir %s\n", dir);
	exits(0);
}
