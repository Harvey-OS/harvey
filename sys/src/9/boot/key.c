#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

static long	finddosfile(int, char*);

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
	int fd, safeoff, safelen;
	char buf[1024];
	Nvrsafe *safe;
	char password[20];

	USED(islocal);
	USED(mp);

	safe = (Nvrsafe*)buf;
	safelen = sizeof(Nvrsafe);
	safeoff = 0;

	if(strcmp(cputype, "sparc") == 0){
		fd = open("#r/nvram", ORDWR);
		safeoff = 1024+850;
	} else if(strcmp(cputype, "386") == 0 || strcmp(cputype, "alpha") == 0){
		fd = open("#S/sdC0/nvram", ORDWR);
		if(fd < 0){
			fd = open("#S/sdC0/9fat", ORDWR);
			if(fd >= 0){
				safeoff = finddosfile(fd, "plan9.nvr");
				if(safeoff < 0){
					close(fd);
					fd = -1;
				}
				safelen = 512;
			}
		}
		if(fd < 0)
			fd = open("#S/sd00/nvram", ORDWR);
		if(fd < 0){
			fd = open("#f/fd0disk", ORDWR);
			if(fd < 0)
				fd = open("#f/fd1disk", ORDWR);
			if(fd >= 0){
				safeoff = finddosfile(fd, "plan9.nvr");
				if(safeoff < 0){
					close(fd);
					fd = -1;
				}
				safelen = 512;
			}
		}
	} else {
		fd = open("#r/nvram", ORDWR);
		safeoff = 1024+900;
	}

	if(fd < 0
	|| seek(fd, safeoff, 0) < 0
	|| read(fd, buf, safelen) != safelen){
		memset(safe, 0, sizeof(safe));
		warning("can't read nvram");
	}
	check(safe->machkey, DESKEYLEN, safe->machsum, "bad nvram key");
	check(safe->authid, NAMELEN, safe->authidsum, "bad authentication id");
	check(safe->authdom, DOMLEN, safe->authdomsum, "bad authentication domain");
	if(kflag){
		do
			getpasswd(password, sizeof password);
		while(!passtokey(safe->machkey, password));
		outin("authid", safe->authid, sizeof(safe->authid));
		outin("authdom", safe->authdom, sizeof(safe->authdom));
		safe->machsum = nvcsum(safe->machkey, DESKEYLEN);
		safe->authidsum = nvcsum(safe->authid, sizeof(safe->authid));
		safe->authdomsum = nvcsum(safe->authdom, sizeof(safe->authdom));
		if(seek(fd, safeoff, 0) < 0
		|| write(fd, buf, safelen) != safelen)
			warning("can't write key to nvram");
	}
	close(fd);

	/* set host's key */
	if(writefile("#c/key", safe->machkey, DESKEYLEN) < 0)
		warning("#c/key");

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", safe->authid, strlen(safe->authid)) < 0)
		warning("#c/hostowner");

	/* set host's domain */
	if(*safe->authdom == 0)
		strcpy(safe->authdom, "plan9");
	if(writefile("#c/hostdomain", safe->authdom, strlen(safe->authdom)) < 0)
		warning("#c/hostdomain");
}

typedef struct Dosboot	Dosboot;
struct Dosboot{
	uchar	magic[3];	/* really an xx86 JMP instruction */
	uchar	version[8];
	uchar	sectsize[2];
	uchar	clustsize;
	uchar	nresrv[2];
	uchar	nfats;
	uchar	rootsize[2];
	uchar	volsize[2];
	uchar	mediadesc;
	uchar	fatsize[2];
	uchar	trksize[2];
	uchar	nheads[2];
	uchar	nhidden[4];
	uchar	bigvolsize[4];
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	type[8];
};
#define	GETSHORT(p) (((p)[1]<<8) | (p)[0])
#define	GETLONG(p) ((GETSHORT((p)+2) << 16) | GETSHORT((p)))

typedef struct Dosdir	Dosdir;
struct Dosdir
{
	char	name[8];
	char	ext[3];
	uchar	attr;
	uchar	reserved[10];
	uchar	time[2];
	uchar	date[2];
	uchar	start[2];
	uchar	length[4];
};

static char*
dosparse(char *from, char *to, int len)
{
	char c;

	memset(to, ' ', len);
	if(from == 0)
		return 0;
	while(len-- > 0){
		c = *from++;
		if(c == '.')
			return from;
		if(c == 0)
			break;
		if(c >= 'a' && c <= 'z')
			*to++ = c + 'A' - 'a';
		else
			*to++ = c;
	}
	return 0;
}

/*
 *  return offset of first file block
 *
 *  This is a very simplistic dos file system.  It only
 *  works on floppies, only looks in the root, and only
 *  returns a pointer to the first block of a file.
 *
 *  This exists for cpu servers that have no hard disk
 *  or nvram to store the key on.
 *
 *  Please don't make this any smarter: it stays resident
 *  and I'ld prefer not to waste the space on something that
 *  runs only at boottime -- presotto.
 */
static long
finddosfile(int fd, char *file)
{
	uchar secbuf[512];
	char name[8];
	char ext[3];
	Dosboot	*b;
	Dosdir *root, *dp;
	int nroot, sectsize, rootoff, rootsects, n;

	/* dos'ize file name */
	file = dosparse(file, name, 8);
	dosparse(file, ext, 3);

	/* read boot block, check for sanity */
	b = (Dosboot*)secbuf;
	if(read(fd, secbuf, sizeof(secbuf)) != sizeof(secbuf))
		return -1;
	if(b->magic[0] != 0xEB || b->magic[1] != 0x3C || b->magic[2] != 0x90)
		return -1;
	sectsize = GETSHORT(b->sectsize);
	if(sectsize != 512)
		return -1;
	rootoff = (GETSHORT(b->nresrv) + b->nfats*GETSHORT(b->fatsize)) * sectsize;
	if(seek(fd, rootoff, 0) < 0)
		return -1;
	nroot = GETSHORT(b->rootsize);
	rootsects = (nroot*sizeof(Dosdir)+sectsize-1)/sectsize;
	if(rootsects <= 0 || rootsects > 64)
		return -1;

	/* 
	 *  read root. it is contiguous to make stuff like
	 *  this easier
	 */
	root = malloc(rootsects*sectsize);
	if(read(fd, root, rootsects*sectsize) != rootsects*sectsize)
		return -1;
	n = -1;
	for(dp = root; dp < &root[nroot]; dp++)
		if(memcmp(name, dp->name, 8) == 0 && memcmp(ext, dp->ext, 3) == 0){
			n = GETSHORT(dp->start);
			break;
		}
	free(root);

	if(n < 0)
		return -1;

	/*
	 *  dp->start is in cluster units, not sectors.  The first
	 *  cluster is cluster 2 which starts immediately after the
	 *  root directory
	 */
	return rootoff + rootsects*sectsize + (n-2)*sectsize*b->clustsize;
}
