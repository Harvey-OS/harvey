#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

typedef union Hblock Hblock;

#define TBLOCK	512
#define NAMSIZ	100
union	Hblock
{
	char	dummy[TBLOCK];
	struct	header
	{
		char	name[NAMSIZ];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[NAMSIZ];
	} dbuf;
};

static int getdir(Hblock *hb, Dir *sp);
static int checksum(Hblock *hb);

uchar*
tarlookup(uchar *addr, char *file, int *dlen)
{
	Hblock *hp;
	Dir dir;

	hp = (Hblock*)addr;
	while(getdir(hp, &dir) != 0) {
		if(strcmp(file, hp->dbuf.name) == 0) {
			*dlen = dir.length;
			return (uchar*)(hp+1);
		}
		hp += (dir.length+TBLOCK-1)/TBLOCK + 1;
	}
	return 0;
}

static int
getdir(Hblock *hp, Dir *sp)
{
	int chksum;

	if (hp->dbuf.name[0] == '\0')
		return 0;
	sp->length = strtol(hp->dbuf.size, 0, 8);
	sp->mtime = strtol(hp->dbuf.mtime, 0, 8);
	chksum = strtol(hp->dbuf.chksum, 0, 8);
	if (chksum != checksum(hp)) {
		print("directory checksum error\n");
		return 0;
	}	
	return 1;
}

static int
checksum(Hblock *hp)
{
	int i;
	char *cp;

	i = 0;
	for (cp = hp->dummy; cp < &hp->dummy[TBLOCK]; cp++) {
		if(cp < hp->dbuf.chksum || cp >= &hp->dbuf.chksum[sizeof(hp->dbuf.chksum)])
			i += *cp & 0xff;
		else
			i += ' ' & 0xff;
	}
	return(i);
}
