#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

int
getsafe(char *field, int len, uchar *sum, char *file)
{
	char buf[64];

	if(nvcsum(field, len) != *sum){
		if(readfile(file, buf, sizeof(buf)) < 0){
			memset(field, 0, len);
			return -1;
		}
	}
	return 0;
}

void
outin(char *prompt, char *buf, int len)
{
	int n;
	char b[64];

	if(len >= sizeof(b))
		len = sizeof(b) - 1;
	print("%s[%s]: ", prompt, buf);
	n = read(0, b, len);
	if(n <= 0)
		exits(0);
	b[n-1] = 0;
	if(n > 1)
		strcpy(buf, b);
}

void
main(void)
{
	int fd;
	Nvrsafe safe;

	fd = open("#r/nvram", ORDWR);
	if(fd < 0
	|| seek(fd, 1024+900, 0) < 0
	|| read(fd, &safe, sizeof safe) != sizeof safe){
		memset(&safe, 0, sizeof(safe));
		fprint(2, "wrkey: can't read nvram: %r\n\n");
	}

	if(getsafe(safe.machkey, DESKEYLEN, &safe.machsum, "#c/key") < 0)
		fprint(2, "wrkey: bad nvram key\n");
	if(getsafe(safe.authid, NAMELEN, &safe.authidsum, "#c/hostowner") < 0)
		fprint(2, "wrkey: bad authentication id\n");
	if(getsafe(safe.authdom, DOMLEN, &safe.authdomsum, "#c/hostdomain") < 0)
		fprint(2, "wrkey: bad authentication domain\n");

	getpass(safe.machkey, 1);
	outin("authid", safe.authid, sizeof(safe.authid));
	outin("authdom", safe.authdom, sizeof(safe.authdom));

	safe.machsum = nvcsum(safe.machkey, DESKEYLEN);
	safe.authidsum = nvcsum(safe.authid, sizeof(safe.authid));
	safe.authdomsum = nvcsum(safe.authdom, sizeof(safe.authdom));
	if(seek(fd, 1024+900, 0) < 0
	|| write(fd, &safe, sizeof safe) != sizeof safe)
		fprint(2, "wrkey: can't write nvram: %r\n");
	close(fd);

	/* set host's key */
	if(writefile("#c/key", safe.machkey, DESKEYLEN) < 0)
		fprint(2, "wrkey: writing #c/key: %r\n");

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", safe.authid, strlen(safe.authid)) < 0)
		fprint(2, "wrkey: writing #c/hostowner: %r\n");

	/* set host's domain */
	if(writefile("#c/hostdomain", safe.authdom, strlen(safe.authdom)) < 0)
		fprint(2, "wrkey: writing #c/hostdomain: %r\n");
	exits(0);
}
