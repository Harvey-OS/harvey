#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "tapefs.h"

/*
 * File system for tar tapes (read-only)
 */

#define TBLOCK	512
#define NBLOCK	40	/* maximum blocksize */
#define DBLOCK	20	/* default blocksize */
#define TNAMSIZ	100

union hblock {
	char dummy[TBLOCK];
	char tbuf[Maxbuf];
	struct header {
		char name[TNAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[TNAMSIZ];
	} dbuf;
} dblock;

int	tapefile;
int	checksum(void);

void
populate(char *name)
{
	long blkno, isabs, chksum, linkflg;
	Fileinf f;

	tapefile = open(name, OREAD);
	if (tapefile<0)
		error("Can't open argument file");
	replete = 1;
	for (blkno = 0;;) {
		seek(tapefile, TBLOCK*blkno, 0);
		if (read(tapefile, dblock.dummy, sizeof(dblock.dummy))<sizeof(dblock.dummy))
			break;
		if (dblock.dbuf.name[0]=='\0')
			break;
		f.addr = (void*)(blkno+1);
		f.mode = strtoul(dblock.dbuf.mode, 0, 8);
		f.uid = strtoul(dblock.dbuf.uid, 0, 8);
		f.gid = strtoul(dblock.dbuf.gid, 0, 8);
		f.size = strtoul(dblock.dbuf.size, 0, 8);
		f.mdate = strtoul(dblock.dbuf.mtime, 0, 8);
		chksum = strtoul(dblock.dbuf.chksum, 0, 8);
		linkflg = dblock.dbuf.linkflag=='s' || dblock.dbuf.linkflag=='1';
		isabs = dblock.dbuf.name[0]=='/';
		if (chksum != checksum()){
			fprint(1, "bad checksum on %.28s\n", dblock.dbuf.name);
			abort();
		}
		if (linkflg) {
			/*fprint(2, "link %s->%s skipped\n", dblock.dbuf.name,
			   dblock.dbuf.linkname);*/
			f.size = 0;
			blkno += 1;
			continue;
		}
		f.name = dblock.dbuf.name+isabs;
		if (f.name[0]=='\0')
			fprint(1, "null name skipped\n");
		else
			poppath(f, 1);
		blkno += 1 + (f.size+TBLOCK-1)/TBLOCK;
	}
}

void
dotrunc(Ram *r)
{
	USED(r);
}

void
docreate(Ram *r)
{
	USED(r);
}

char *
doread(Ram *r, long off, long cnt)
{

	seek(tapefile, (TBLOCK * (long)r->data)+off, 0);
	if (cnt>sizeof(dblock.tbuf))
		error("read too big");
	read(tapefile, dblock.tbuf, cnt);
	return dblock.tbuf;
}

void
popdir(Ram *r)
{
	USED(r);
}

void
dowrite(Ram *r, char *buf, long off, long cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}

int
checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp&0xff;
	return(i);
}
