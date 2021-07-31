#include <u.h>
#include <libc.h>

void
main(int argc, char **argv)
{
	int fd, n, m;
	char buf[1024], dir[512], *str;

	fd = open("/dev/acme/ctl", OWRITE);
	if(fd < 0)
		exits(0);
	getwd(dir, 512);
	strcpy(buf, "name ");
	strcpy(buf+5, dir);
	n = strlen(buf);
	if(n>0 && buf[n-1]!='/')
		buf[n++] = '/';
	buf[n++] = '-';
	if(argc > 1)
		str = argv[1];
	else
		str = "rc";
	m = strlen(str);
	strcpy(buf+n, str);
	n += m;
	buf[n++] = '\n';
	write(fd, buf, n);
	strcpy(buf, "dumpdir ");
	strcpy(buf+8, dir);
	strcat(buf, "\n");
	write(fd, buf, strlen(buf));
	exits(0);
}
