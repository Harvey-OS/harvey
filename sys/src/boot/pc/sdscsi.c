#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "error.h"

#include "sd.h"

static int
scsitest(SDreq* r)
{
	r->write = 0;
	memset(r->cmd, 0, sizeof(r->cmd));
	r->cmd[1] = r->lun<<5;
	r->clen = 6;
	r->data = nil;
	r->dlen = 0;
	r->flags = 0;

	r->status = ~0;

// cgascreenputs("A", 1);
	return r->unit->dev->ifc->rio(r);
}

int
scsiverify(SDunit* unit)
{
	static SDreq *r;
	int i, status;
	static uchar *inquiry;

	if((r = sdmalloc(r, sizeof(SDreq))) == nil)
		return 0;

	if((inquiry = sdmalloc(inquiry, sizeof(unit->inquiry))) == nil)
		return 0;

	r->unit = unit;
	r->lun = 0;		/* ??? */

	memset(unit->inquiry, 0, sizeof(unit->inquiry));
	r->write = 0;
	r->cmd[0] = 0x12;
	r->cmd[1] = r->lun<<5;
	r->cmd[4] = sizeof(unit->inquiry)-1;
	r->clen = 6;
	r->data = inquiry;
	r->dlen = sizeof(unit->inquiry)-1;
	r->flags = 0;

	r->status = ~0;
// cgascreenputs("B", 1);
	if(unit->dev->ifc->rio(r) != SDok){
		return 0;
	}
	memmove(unit->inquiry, inquiry, r->dlen);

	SET(status);
	for(i = 0; i < 3; i++){
		while((status = scsitest(r)) == SDbusy)
			;
		if(status == SDok || status != SDcheck)
			break;
		if(!(r->flags & SDvalidsense))
			break;
		if((r->sense[2] & 0x0F) != 0x02)
			continue;

		/*
		 * Unit is 'not ready'.
		 * If it is in the process of becoming ready or needs
		 * an initialising command, set status so it will be spun-up
		 * below.
		 * If there's no medium, that's OK too, but don't
		 * try to spin it up.
		 */
		if(r->sense[12] == 0x04){
			if(r->sense[13] == 0x02 || r->sense[13] == 0x01){
				status = SDok;
				break;
			}
		}
		if(r->sense[12] == 0x3A)
			break;
	}

	if(status == SDok){
		/*
		 * Try to ensure a direct-access device is spinning.
		 * Ignore the result.
		 */
		if((unit->inquiry[0] & 0x1F) == 0){
			memset(r->cmd, 0, sizeof(r->cmd));
			r->write = 0;
			r->cmd[0] = 0x1B;
			r->cmd[1] = r->lun<<5;
			r->cmd[4] = 1;
			r->clen = 6;
			r->data = nil;
			r->dlen = 0;
			r->flags = 0;

			r->status = ~0;
			unit->dev->ifc->rio(r);
		}
		return 1;
	}
	return 0;
}

int
return0(void*)
{
	return 0;
}

static int
scsirio(SDreq* r)
{
	/*
	 * Perform an I/O request, returning
	 *	-1	failure
	 *	 0	ok
	 *	 2	retry
	 * The contents of r may be altered so the
	 * caller should re-initialise if necesary.
	 */
	r->status = ~0;
// cgascreenputs("C", 1);
	switch(r->unit->dev->ifc->rio(r)){
	default:
		return -1;
	case SDcheck:
		if(!(r->flags & SDvalidsense))
			return -1;
		switch(r->sense[2] & 0x0F){
		case 0x00:		/* no sense */
		case 0x01:		/* recovered error */
			return 2;
		case 0x06:		/* check condition */
			/*
			 * 0x28 - not ready to ready transition,
			 *	  medium may have changed.
			 * 0x29 - power on or some type of reset.
			 */
			if(r->sense[12] == 0x28 && r->sense[13] == 0)
				return 2;
			if(r->sense[12] == 0x29)
				return 2;
			return -1;
		case 0x02:		/* not ready */
			/*
			 * If no medium present, bail out.
			 * If unit is becoming ready, rather than not
			 * not ready, wait a little then poke it again. 							 */
			if(r->sense[12] == 0x3A)
				return -1;
			if(r->sense[12] != 0x04 || r->sense[13] != 0x01)
				return -1;

			tsleep(nil, return0, 0, 500);
			scsitest(r);
			return 2;
		default:
			return -1;
		}
		return -1;
	case SDok:
		return 0;
	}
	return -1;
}

int
scsionline(SDunit* unit)
{
	int ok;
	static SDreq *r;
	static uchar *p;

	if((r = sdmalloc(r, sizeof(SDreq))) == nil)
		return 0;

	if((p = sdmalloc(p, 8)) == nil)
		return 0;

	ok = 0;

	r->unit = unit;
	r->lun = 0;				/* ??? */
	for(;;){
		/*
		 * Read-capacity is mandatory for DA, WORM, CD-ROM and
		 * MO. It may return 'not ready' if type DA is not
		 * spun up, type MO or type CD-ROM are not loaded or just
		 * plain slow getting their act together after a reset.
		 */
		r->write = 0;
		memset(r->cmd, 0, sizeof(r->cmd));
		r->cmd[0] = 0x25;
		r->cmd[1] = r->lun<<5;
		r->clen = 10;
		r->data = p;
		r->dlen = 8;
		r->flags = 0;
	
		r->status = ~0;
// cgascreenputs("F", 1);
		switch(scsirio(r)){
		default:
			break;
		case 0:
			unit->sectors = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
			/*
			 * Read-capacity returns the LBA of the last sector,
			 * therefore the number of sectors must be incremented.
			 */
			unit->sectors++;
			unit->secsize = (p[4]<<24)|(p[5]<<16)|(p[6]<<8)|p[7];
			ok = 1;
			break;
		case 2:
			continue;
		}
		break;
	}

	return ok;
}

int
scsiexec(SDunit* unit, int write, uchar* cmd, int clen, void* data, int* dlen)
{
	static SDreq *r;
	int status;

	if((r = sdmalloc(r, sizeof(SDreq))) == nil)
		return SDmalloc;

	r->unit = unit;
	r->lun = cmd[1]>>5;		/* ??? */
	r->write = write;
	memmove(r->cmd, cmd, clen);
	r->clen = clen;
	r->data = data;
	if(dlen)
		r->dlen = *dlen;
	r->flags = 0;

	r->status = ~0;

	/*
	 * Call the device-specific I/O routine.
	 * There should be no calls to 'error()' below this
	 * which percolate back up.
	 */
// cgascreenputs("D", 1);
	switch(status = unit->dev->ifc->rio(r)){
	case SDok:
		if(dlen)
			*dlen = r->rlen;
		/*FALLTHROUGH*/
	case SDcheck:
		/*FALLTHROUGH*/
	default:
		/*
		 * It's more complicated than this. There are conditions
		 * which are 'ok' but for which the returned status code
		 * is not 'SDok'.
		 * Also, not all conditions require a reqsense, might
		 * need to do a reqsense here and make it available to the
		 * caller somehow.
		 *
		 * Mañana.
		 */
		break;
	}

	return status;
}

long
scsibio(SDunit* unit, int lun, int write, void* data, long nb, long bno)
{
	static SDreq *r;
	long rlen;

	if((r = sdmalloc(r, sizeof(SDreq))) == nil)
		return SDmalloc;

	r->unit = unit;
	r->lun = lun;
again:
	r->write = write;
	if(write == 0)
		r->cmd[0] = 0x28;
	else
		r->cmd[0] = 0x2A;
	r->cmd[1] = (lun<<5);
	r->cmd[2] = bno>>24;
	r->cmd[3] = bno>>16;
	r->cmd[4] = bno>>8;
	r->cmd[5] = bno;
	r->cmd[6] = 0;
	r->cmd[7] = nb>>8;
	r->cmd[8] = nb;
	r->cmd[9] = 0;
	r->clen = 10;
	r->data = data;
	r->dlen = nb*unit->secsize;
	r->flags = 0;

	r->status = ~0;
// cgascreenputs("E", 1);
	switch(scsirio(r)){
	default:
		rlen = -1;
		break;
	case 0:
		rlen = r->rlen;
		break;
	case 2:
		rlen = -1;
		if(!(r->flags & SDvalidsense))
			break;
		switch(r->sense[2] & 0x0F){
		default:
			break;
		case 0x06:		/* check condition */
			/*
			 * Check for a removeable media change.
			 * If so, mark it and zap the geometry info
			 * to force an online request.
			 */
			if(r->sense[12] != 0x28 || r->sense[13] != 0)
				break;
			if(unit->inquiry[1] & 0x80){
				unit->sectors = 0;
			}
			break;
		case 0x02:		/* not ready */
			/*
			 * If unit is becoming ready,
			 * rather than not not ready, try again.
			 */
			if(r->sense[12] == 0x04 && r->sense[13] == 0x01)
				goto again;
			break;
		}
		break;
	}

	return rlen;
}

SDev*
scsiid(SDev* sdev, SDifc* ifc)
{
	static char idno[16] = "0123456789abcdef";
	static char *p = idno;

	while(sdev){
		if(sdev->ifc == ifc){
			sdev->idno = *p++;
			if(p >= &idno[sizeof(idno)])
				break;
		}
		sdev = sdev->next;
	}

	return nil;
}
