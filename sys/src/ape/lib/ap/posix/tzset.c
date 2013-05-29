#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define TZFILE	"/etc/TZ"

static char TZ[128];
static char std[32] = "GMT0";
static char dst[32];
char *tzname[2] = {
	std, dst
};
time_t tzoffset, tzdstoffset;
int tzdst = 0;

static int
offset(char *env, time_t *off)
{
	int n, sign;
	size_t len, retlen;

	retlen = 0;
	sign = 1;
	/*
	 * strictly, no sign is allowed in the 'time' part of the
	 * dst start/stop rules, but who cares?
	 */
	if (*env == '-' || *env == '+') {
		if (*env++ == '-')
			sign = -1;
		retlen++;
	}
	if ((len = strspn(env, ":0123456789")) == 0)
		return 0;
	retlen += len;
	for (*off = 0; len && isdigit(*env); len--)		/* hours */
		*off = *off*10 + (*env++ - '0')*60*60;
	if (len) {
		if (*env++ != ':')
			return 0;
		len--;
	}
	for (n = 0; len && isdigit(*env); len--)		/* minutes */
		n = n*10 + (*env++ - '0')*60;
	*off += n;
	if (len) {
		if (*env++ != ':')
			return 0;
		len--;
	}
	for (n = 0; len && isdigit(*env); len--)		/* seconds */
		n = n*10 + (*env++ - '0');
	*off = (*off + n)*sign;
	return retlen;
}

/*
 * TZ=stdoffset[dst[offset][,start[/time],end[/time]]]
 */
void
tzset(void)
{
	char *env, *p, envbuf[128];
	int fd, i;
	size_t len, retlen;
	time_t off;

	/*
	 * get the TZ environment variable and check for validity.
	 * the implementation-defined manner for dealing with the
	 * leading ':' format is to reject it.
	 * if it's ok, stash a copy away for quick comparison next time.
	 */
	if ((env = getenv("TZ")) == 0) {
		if ((fd = open(TZFILE, O_RDONLY)) == -1)
			return;
		if (read(fd, envbuf, sizeof(envbuf)-1) == -1) {
			close(fd);
			return;
		}
		close(fd);
		for (i = 0; i < sizeof(envbuf); i++) {
			if (envbuf[i] != '\n')
				continue;
			envbuf[i] = '\0';
			break;
		}
		env = envbuf;
	}
	if (strcmp(env, TZ) == 0)
		return;
	if (*env == 0 || *env == ':')
		return;
	strncpy(TZ, env, sizeof(TZ)-1);
	TZ[sizeof(TZ)-1] = 0;
	/*
	 * get the 'std' string.
	 * before committing, check there is a valid offset.
	 */
	if ((len = strcspn(env, ":0123456789,-+")) == 0)
		return;
	if ((retlen = offset(env+len, &off)) == 0)
		return;
	for (p = std, i = len+retlen; i; i--)
		*p++ = *env++;
	*p = 0;
	tzoffset = -off;
	/*
	 * get the 'dst' string (if any).
	 */
	if (*env == 0 || (len = strcspn(env, ":0123456789,-+")) == 0)
		return;
	for (p = dst; len; len--)
		*p++ = *env++;
	*p = 0;
	/*
	 * optional dst offset.
	 * default is one hour.
	 */
	tzdst = 1;
	if (retlen = offset(env+len, &off)) {
		tzdstoffset = -off;
		env += retlen;
	}
	else
		tzdstoffset = tzoffset + 60*60;
	/*
	 * optional rule(s) for start/end of dst.
	 */
	if (*env == 0 || *env != ',' || *(env+1) == 0)
		return;
	env++;
	/*
	 * we could go on...
	 * but why bother?
	 */
	USED(env);
}
