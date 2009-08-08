/*
 * drive disks
 * used to be just scsi disks, and issued scsi commands directly to the host
 * adapter, but now it just does normal i/o.
 */
#include "all.h"

enum { Sectorsz = 512, };		/* usual disk sector size */

typedef	struct	Wren	Wren;
struct	Wren
{
	long	block;			/* size of a block -- from config */
	Devsize	nblock;			/* number of blocks -- from config */
	long	mult;			/* multiplier to get physical blocks */
	Devsize	max;			/* number of logical blocks */

//	char	*sddir;			/* /dev/sdXX name */
};

char *
dataof(char *file)
{
	char *datanm;
	Dir *dir;

	dir = dirstat(file);
	if (dir != nil && dir->mode & DMDIR)
		datanm = smprint("%s/data", file);
	else
		datanm = strdup(file);
	free(dir);
	return datanm;
}

void
wreninit(Device *d)
{
	Wren *dr;
	Dir *dir;

	if(d->private)
		return;
	d->private = dr = malloc(sizeof(Wren));

	if (d->wren.file)
		d->wren.sddata = dataof(d->wren.file);
	else {
		d->wren.sddir = sdof(d);
		d->wren.sddata = smprint("%s/data", d->wren.sddir);
	}

	assert(d->wren.fd <= 0);
	d->wren.fd = open(d->wren.sddata, ORDWR);
	if (d->wren.fd < 0)
		panic("wreninit: can't open %s for %Z: %r", d->wren.sddata, d);

	dr->block = inqsize(d->wren.sddata);
	if(dr->block <= 0 || dr->block >= 16*1024) {
		print("\twreninit %Z block size %ld, setting to %d\n",
			d, dr->block, Sectorsz);
		dr->block = Sectorsz;
	}

	dir = dirfstat(d->wren.fd);
	dr->nblock = dir->length / dr->block;
	free(dir);

	dr->mult = (RBUFSIZE + dr->block - 1) / dr->block;
	dr->max = (dr->nblock + 1) / dr->mult;
	print("\tdisk drive %Z: %,lld %ld-byte sectors, ",
		d, (Wideoff)dr->nblock, dr->block);
	print("%,lld %d-byte blocks\n", (Wideoff)dr->max, RBUFSIZE);
	print("\t\t%ld multiplier\n", dr->mult);
}

Devsize
wrensize(Device *d)
{
	return ((Wren *)d->private)->max;
}

int
wrenread(Device *d, Off b, void *c)
{
	int r = 0;
	Wren *dr = d->private;

	if (dr == nil)
		panic("wrenread: no drive (%Z) block %lld", d, (Wideoff)b);
	if(b >= dr->max) {
		print("wrenread: block out of range %Z(%lld)\n", d, (Wideoff)b);
		r = 0x040;
	} else if (pread(d->wren.fd, c, RBUFSIZE, (vlong)b*RBUFSIZE) != RBUFSIZE) {
		print("wrenread: error on %Z(%lld): %r\n", d, (Wideoff)b);
		cons.nwrenre++;
		r = 1;
	}
	return r;
}

int
wrenwrite(Device *d, Off b, void *c)
{
	int r = 0;
	Wren *dr = d->private;

	if (dr == nil)
		panic("wrenwrite: no drive (%Z) block %lld", d, (Wideoff)b);
	if(b >= dr->max) {
		print("wrenwrite: block out of range %Z(%lld)\n",
			d, (Wideoff)b);
		r = 0x040;
	} else if (pwrite(d->wren.fd, c, RBUFSIZE, (vlong)b*RBUFSIZE) != RBUFSIZE) {
		print("wrenwrite: error on %Z(%lld): %r\n", d, (Wideoff)b);
		cons.nwrenwe++;
		r = 1;
	}
	return r;
}
