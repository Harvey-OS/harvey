#include <u.h>
#include <libc.h>

void
main(void)
{
	int fd, p[2];
	char buf[8192], n;

	pipe(p);
	fd = create("/srv/log", OWRITE, 0666);
	fprint(fd, "%d", p[0]);
	close(fd);
	close(p[0]);
	while((n = read(p[1], buf, sizeof buf)) >= 0)
		write(1, buf, n);
}
