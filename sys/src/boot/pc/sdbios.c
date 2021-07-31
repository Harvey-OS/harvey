/*
 * boot driver for BIOS devices with partitions
 * devbios must be called first
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "error.h"

#include "sd.h"
#include "fs.h"

long	biosread(Fs *, void *, long);
vlong	biosseek(Fs *fs, vlong off);

extern SDifc sdbiosifc;
extern int onlybios0, biosinited;

int
biosverify(SDunit* )
{
	if (onlybios0 || !biosinited)
		return 0;
	return 1;
}

int
biosonline(SDunit* unit)
{
	if (onlybios0 || !biosinited || !unit)
		return 0;
	unit->sectors = 1UL << 30;	/* a bunch */
	unit->secsize = 512;		/* conventional */
	return 1;
}

static int
biosrio(SDreq* r)
{
	int nb;
	long got;
	vlong len, off;
	uchar *p;
	Fs fs;			/* just for fs->dev, which is zero */

	if (onlybios0 || !biosinited)
		return SDeio;
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
	case 0x08:			/* read */
	case 0x28:			/* read */
		if (r->cmd[0] == 0x08)
			panic("biosrio: 0x08 read op\n");
		off = r->cmd[2]<<24 | r->cmd[3]<<16 | r->cmd[4]<<8 | r->cmd[5];
		nb = r->cmd[7]<<8 | r->cmd[8];	/* often 4 */
		USED(nb);		/* is nb*512 == r->dlen? */
		memset(&fs, 0, sizeof fs);
		biosseek(&fs, off*512);
		got = biosread(&fs, r->data, r->dlen);
		if (got < 0)
			r->status = SDeio;
		else
			r->rlen = got;
		break;
	case 0x0A:			/* write */
	case 0x2A:			/* write */
		r->status = SDeio;	/* boot programs don't write */
		break;
	case 0x25:			/* read capacity */
		/*
		 * Read capacity returns the LBA of the last sector.
		 */
		len = r->unit->sectors - 1;
		p = r->data;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		len = r->unit->secsize;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p = len;
		r->data = (char *)r->data + 8;
		return SDok;
	case 0x9E:			/* long read capacity */
		/*
		 * Read capacity returns the LBA of the last sector.
		 */
		len = r->unit->sectors - 1;
		p = r->data;
		*p++ = len>>56;
		*p++ = len>>48;
		*p++ = len>>40;
		*p++ = len>>32;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		len = r->unit->secsize;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p = len;
		r->data = (char *)r->data + 8;
		return SDok;
	/* ignore others */
	}
	return r->status;
}

SDev*
biosid(SDev* sdev)
{
	for (; sdev; sdev = sdev->next)
		if (sdev->ifc == &sdbiosifc)
			sdev->idno = 'B';
	return sdev;
}

static SDev*
biospnp(void)
{
	SDev *sdev;

	/* 9pxeload can't use bios int 13 calls; they wedge the machine */
	if (pxe || getconf("*nobiosload") != nil || onlybios0 || !biosinited)
		return nil;
	if((sdev = malloc(sizeof(SDev))) != nil) {
		sdev->ifc = &sdbiosifc;
		sdev->index = -1;
		sdev->nunit = 1;
	}
	return sdev;
}

SDifc sdbiosifc = {
	"bios",				/* name */

	biospnp,			/* pnp */
	nil,				/* legacy */
	biosid,				/* id */
	nil,				/* enable */
	nil,				/* disable */

	biosverify,			/* verify */
	biosonline,			/* online */
	biosrio,			/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	scsibio,			/* bio */
};
