/*
 * read-only sd driver for BIOS devices with partitions.
 * will probably only work with bootstrap kernels, as the normal kernel
 * deals directly with the clock and disk controllers, which seems
 * to confuse many BIOSes.
 *
 * devbios must be initialised first and no disks may be accessed
 * via non-BIOS means.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/error.h"
#include	"../port/netif.h"
#include	"../port/sd.h"
#include	"dosfs.h"
#include	<disk.h>

long	biosread0(Bootfs *, void *, long);
vlong	biosseek(Bootfs *fs, vlong off);

extern SDifc sdbiosifc;
extern int biosndevs;

uchar *
putbeul(ulong ul, uchar *p)
{
	*p++ = ul >> 24;
	*p++ = ul >> 16;
	*p++ = ul >> 8;
	*p++ = ul;
	return p;
}

uchar *
putbeuvl(uvlong uvl, uchar *p)
{
	*p++ = uvl >> 56;
	*p++ = uvl >> 48;
	*p++ = uvl >> 40;
	*p++ = uvl >> 32;
	*p++ = uvl >> 24;
	*p++ = uvl >> 16;
	*p++ = uvl >> 8;
	*p++ = uvl;
	return p;
}

int
biosverify(SDunit* )
{
	if (!biosinited)
		return 0;
	return 1;
}

int
biosonline(SDunit* unit)
{
	uint subno = unit->subno;

	if (!biosinited)
		panic("sdbios: biosonline: sdbios not inited");
	if (unit == nil)
		return 0;
	unit->secsize = biossectsz(subno);
	if (unit->secsize <= 0) {
		print("sdbios: biosonline: implausible sector size on medium\n");
		return 0;
	}
	unit->sectors = biossize(subno);
	if (unit->sectors <= 0) {
		unit->sectors = 0;
		print("sdbios: biosonline: no sectors on medium\n");
		return 0;
	}
	return 1;
}

static int
biosrio(SDreq* r)
{
	int nb;
	long got;
	vlong off;
	uchar *p;
	Bootfs fs;			/* just for fs->dev, which is zero */
	SDunit *unit;

	if (!biosinited)
		return SDeio;
	unit = r->unit;
	/*
	 * Most SCSI commands can be passed unchanged except for
	 * the padding on the end. The few which require munging
	 * are not used internally. Mode select/sense(6) could be
	 * converted to the 10-byte form but it's not worth the
	 * effort. Read/write(6) are easy.
	 */
	r->rlen = 0;
	r->status = SDok;
	switch(r->cmd[0]){
	case ScmdRead:
	case ScmdExtread:
		if (r->cmd[0] == ScmdRead)
			panic("biosrio: ScmdRead read op");
		off = r->cmd[2]<<24 | r->cmd[3]<<16 | r->cmd[4]<<8 | r->cmd[5];
		nb =  r->cmd[7]<<8  | r->cmd[8];	/* often 4 */
		USED(nb);		/* is nb*unit->secsize == r->dlen? */
		memset(&fs, 0, sizeof fs);
		biosseek(&fs, off * unit->secsize);
		got = biosread0(&fs, r->data, r->dlen);
		if (got < 0)
			r->status = SDeio;
		else
			r->rlen = got;
		break;
	case ScmdWrite:
	case ScmdExtwrite:
		r->status = SDeio;	/* boot programs don't write */
		break;

		/*
		 * Read capacity returns the LBA of the last sector.
		 */
	case ScmdRcapacity:
		p = putbeul(r->unit->sectors - 1, r->data);
		r->data = putbeul(r->unit->secsize, p);
		return SDok;
	case ScmdRcapacity16:
		p = putbeuvl(r->unit->sectors - 1, r->data);
		r->data = putbeul(r->unit->secsize, p);
		return SDok;
	/* ignore others */
	}
	return r->status;
}

/* this is called between biosreset and biosinit */
static SDev*
biospnp(void)
{
	SDev *sdev;

	if (!biosinited)
		panic("sdbios: biospnp: bios devbios not yet inited");
	if((sdev = malloc(sizeof(SDev))) != nil) {
		sdev->ifc = &sdbiosifc;
		sdev->idno = 'B';
		sdev->nunit = biosndevs;
		iprint("sdbios: biospnp: %d unit(s) at sd%C0\n",
			sdev->nunit, sdev->idno);
	}
	return sdev;
}

SDifc sdbiosifc = {
	"bios",				/* name */

	biospnp,			/* pnp */
	nil,				/* legacy */
	nil,				/* enable */
	nil,				/* disable */

	biosverify,			/* verify */
	biosonline,			/* online */
	biosrio,			/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	scsibio,			/* bio */
	nil,				/* probe */
	nil,				/* clear */
	nil,				/* rtopctl */
	nil,				/* wtopctl */
};
