#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	Plumbmsg *m;
	int fd;

	fd = plumbopen("audioplay", OREAD);
	if (fd < 0)
		sysfatal("port audioplay: %r");
	for (;;) {
		m = plumbrecv(fd);
		if (m == nil)
			sysfatal("plumrecv: %r");
		
		plumbfree(m);
	}
}
