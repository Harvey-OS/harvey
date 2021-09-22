#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[512], *p, *e;

	p = buf;
	e = buf + sizeof buf;

	p = seprint(p, e, "#!/bin/rc\n");
	p = seprint(p, e, "echo hel\xc0;echo 2nd line\n");
	p = seprint(p, e, "echo 3 byte \xe0α\n");
	p = seprint(p, e, "echo 1 byte \xe0;echo y\n");
	p = seprint(p, e, "echo 3 byte \xf0\x92\x80y\n");
	p = seprint(p, e, "echo 1 byte\xe0");
	write(1, buf, p - buf);
	exits(0);
}
