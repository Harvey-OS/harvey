#include "lib.h"
#include "sys9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "dir.h"

int
chown(const char *path, uid_t owner, gid_t group)
{
	int n;
	int num;
	char *nam;
	char cd[DIRLEN];
	Dir d;

	n = -1;
	if(_STAT(path, cd) < 0)
		_syserrno();
	else{
		convM2D(cd, &d);

		/* check that d.uid matches owner, and find name for group */
		nam = d.uid;
		if(_getpw(&num, &nam, 0)) {
			if(num != owner) {
				/* can't change owner in Plan 9 */
				errno = EPERM;
				return -1;
			}
		} else {
			errno = EINVAL;
			return -1;
		}
		nam = 0;
		num = group;
		if(!_getpw(&num, &nam, 0)) {
			/* couldn't find group */
			errno = EINVAL;
			return -1;
		}
		memset(d.gid, 0, NAMELEN);
		strcpy(d.gid, nam);

		convD2M(&d, cd);
		if(_WSTAT(path, cd) < 0)
			_syserrno();
		else
			n = 0;
	}
	return n;
}
