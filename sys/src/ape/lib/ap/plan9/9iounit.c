#include "lib.h"
#include <string.h>
#include <stdlib.h>
#include <fmt.h>
#include "sys9.h"
#include "dir.h"

/*
 * Format:
  3 r  M    4 (0000000000457def 11 00)   8192      512 /rc/lib/rcmain
 */

static int
getfields(char *str, char **args, int max, int mflag)
{
	char r;
	int nr, intok, narg;

	if(max <= 0)
		return 0;

	narg = 0;
	args[narg] = str;
	if(!mflag)
		narg++;
	intok = 0;
	for(;;) {
		nr = 1;			/* utf bytes in this rune */
		r = *str++;
		if(r == 0)
			break;
		if(r == ' ' || r == '\t'){
			if(narg >= max)
				break;
			*str = 0;
			intok = 0;
			args[narg] = str + nr;
			if(!mflag)
				narg++;
		} else {
			if(!intok && mflag)
				narg++;
			intok = 1;
		}
	}
	return narg;
}
int
_IOUNIT(int fd)
{
	int i, cfd;
	char buf[128], *args[10];

	snprint(buf, sizeof buf, "#d/%dctl", fd);
	cfd = _OPEN(buf, OREAD);
	if(cfd < 0)
		return 0;
	i = _READ(cfd, buf, sizeof buf-1);
	_CLOSE(cfd);
	if(i <= 0)
		return 0;
	buf[i] = '\0';
	if(getfields(buf, args, 10, 1) != 10)
		return 0;
	return atoi(args[7]);
}
