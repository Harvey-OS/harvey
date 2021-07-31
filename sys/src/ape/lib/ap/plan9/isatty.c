#include "lib.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sys9.h"
#include "dir.h"

int
_isatty(int fd)
{
	int t;
	Dir d1, d2;
	char cd[DIRLEN];
	char buf[5+NAMELEN];

	if(_FSTAT(fd, cd) < 0)
		return 0;
	convM2D(cd, &d1);
	if(strncmp(d1.name, "ptty", 4)==0)
		return 1;
	if(_STAT("/dev/cons", cd) < 0)
		return 0;
	convM2D(cd, &d2);

	/*
	 * If we came in through con, /dev/cons is probably #d/0, which
	 * won't match stdin.  Opening #d/0 and fstating it gives the
	 * values of the underlying channel
	 */

	if(d2.type == 'd'){
		strcpy(buf, "#d/");
		strcpy(buf+3, d2.name);
		if((t = _OPEN(buf, 0)) < 0)
			return 0;
		if(_FSTAT(t, cd) < 0){
			_CLOSE(t);
			return 0;
		}
		_CLOSE(t);
		convM2D(cd, &d2);
	}
	return (d1.type == d2.type) && (d1.dev == d2.dev);
}

/* The FD_ISTTY flag is set via _isatty in _fdsetup or open */
int
isatty(fd)
{
	if(_fdinfo[fd].flags&FD_ISTTY)
		return 1;
	else
		return 0;
}
