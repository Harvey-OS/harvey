#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

static void
check(void *x, int len, uchar sum, char *msg)
{
	if(nvcsum(x, len) == sum)
		return;
	memset(x, 0, len);
	kflag = 1;
	warning(msg);
}

/*
 *  get info out of nvram.  since there isn't room in the PC's nvram use
 *  a disk partition there.
 */
void
key(int islocal, Method *mp)
{
	int fd, safeoff;
	Nvrsafe safe;
	char password[20];

	USED(islocal);
	USED(mp);

	if(strcmp(cputype, "sparc") == 0){
		fd = open("#r/nvram", ORDWR);
		safeoff = 1024+850;
	} else if(strcmp(cputype, "386") == 0){
		fd = open("#H/hd0nvram", ORDWR);
		if(fd < 0)
			fd = open("#w/sd0nvram", ORDWR);
		safeoff = 0x0;
	} else {
		fd = open("#r/nvram", ORDWR);
		safeoff = 1024+900;
	}

	if(fd < 0
	|| seek(fd, safeoff, 0) < 0
	|| read(fd, &safe, sizeof safe) != sizeof safe){
		memset(&safe, 0, sizeof(safe));
		warning("can't read nvram");
	}
	check(safe.machkey, DESKEYLEN, safe.machsum, "bad nvram key");
	check(safe.authid, NAMELEN, safe.authidsum, "bad authentication id");
	check(safe.authdom, DOMLEN, safe.authdomsum, "bad authentication domain");
	if(kflag){
		do
			getpasswd(password, sizeof password);
		while(!passtokey(safe.machkey, password));
		outin(0, "authid", safe.authid, sizeof(safe.authid));
		outin(0, "authdom", safe.authdom, sizeof(safe.authdom));
		safe.machsum = nvcsum(safe.machkey, DESKEYLEN);
		safe.authidsum = nvcsum(safe.authid, sizeof(safe.authid));
		safe.authdomsum = nvcsum(safe.authdom, sizeof(safe.authdom));
		if(seek(fd, safeoff, 0) < 0
		|| write(fd, &safe, sizeof safe) != sizeof safe)
			warning("can't write key to nvram");
	}
	close(fd);

	/* set host's key */
	if(writefile("#c/key", safe.machkey, DESKEYLEN) < 0)
		fatal("#c/key");

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", safe.authid, strlen(safe.authid)) < 0)
		fatal("#c/hostowner");

	/* set host's domain */
	if(writefile("#c/hostdomain", safe.authdom, strlen(safe.authdom)) < 0)
		fatal("#c/hostdomain");
}
