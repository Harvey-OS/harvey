#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <auth.h>
#include "authsrv.h"

void
wrbio(char *file, Acctbio *a)
{
	char uinfo[512];
	char sponsors[512];
	int i, fd, n;

	fd = open(file, OWRITE);
	if(fd < 0)
		error("can't open %s", file);
	if(seek(fd, 0, 2) < 0)
		error("can't seek %s", file);

	if(a->name == 0)
		a->name = strdup(a->user);
	if(a->dept == 0)
		a->dept = strdup("1127?");
	if(a->email[0] == 0)
		a->email[0] = strdup(a->user);

	snprint(uinfo, sizeof(uinfo), "%s %s <%s>", a->name, a->dept, a->email[0]);
	n = 0;
	sponsors[0] = 0;
	for(i = 1; i < Nemail; i++){
		if(a->email[i] == 0)
			break;
		n += snprint(sponsors+n, sizeof(sponsors)-n, "<%s>", a->email[i]);
	}

	fprint(fd, "%s		%s		%s\n", a->user, uinfo, sponsors);
	close(fd);
}
