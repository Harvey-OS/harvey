#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "wrap.h"

static Biobuf*
Bopengunzipstream(char *file)
{
	int fd, p[2];
	Biobuf *b;

	if((fd = open(file, OREAD)) < 0)
		sysfatal("gunzip cannot open '%s' for read", file);

	if(pipe(p) < 0)
		sysfatal("pipe");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		sysfatal("rfork gunzip pipe");
	case 0:
		dup(fd, 0);
		dup(p[0], 1);
		dup(p[0], 2);
		close(p[0]);
		close(p[1]);

		execl("/bin/gunzip", "gunzip", 0);
		sysfatal("exec gunzip pipe");

	default:
		close(fd);
		close(p[0]);
	}

	b = emalloc(sizeof(*b));
	Binit(b, p[1], OREAD);
	b->flag = Bmagic;	/* now Bterm will close p[1], free b */
	return b;
}

Biobuf*
Bopengz(char *file)
{
	Biobuf *b;
	int c;

	if((b = Bopen(file, OREAD)) == nil)
		return nil;

	if(Bgetc(b) == 0x1F && ((c = Bgetc(b)) == 0x8B || c == 0x9D)) {
		Bterm(b);
		b = Bopengunzipstream(file);
	}

	return b;
}
