#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include "authcmdlib.h"

void
wrbio(char *file, Acctbio *a)
{
	char buf[1024];
	int i, fd, n;

	fd = open(file, OWRITE);
	if(fd < 0)
		error("can't open %s", file);
	if(seek(fd, 0, 2) < 0)
		error("can't seek %s", file);

	if(a->postid == 0)
		a->postid = "";
	if(a->name == 0)
		a->name = "";
	if(a->dept == 0)
		a->dept = "";
	if(a->email[0] == 0)
		a->email[0] = strdup(a->user);

	n = 0;
	n += snprint(buf+n, sizeof(buf)-n, "%s|%s|%s|%s",
		a->user, a->postid, a->name, a->dept);
	for(i = 0; i < Nemail; i++){
		if(a->email[i] == 0)
			break;
		n += snprint(buf+n, sizeof(buf)-n, "|%s", a->email[i]);
	}
	n += snprint(buf+n, sizeof(buf)-n, "\n");

	write(fd, buf, n);
	close(fd);
}
