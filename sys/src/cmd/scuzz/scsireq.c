#include <u.h>
#include <libc.h>
/*
 * BUGS:
 *	no luns
 *	and incomplete in many other ways
 */
#include "scsireq.h"

enum {
	Debug = 0,
};

/*
 * exabyte tape drives, at least old ones like the 8200 and 8505,
 * are dumb: you have to read the exact block size on the tape,
 * they don't take 10-byte SCSI commands, and various other fine points.
 */
extern int exabyte, force6bytecmds;

static int debug = Debug;

long
SRready(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRrewind(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdRewind;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	if(SRrequest(rp) >= 0){
		rp->offset = 0;
		return 0;
	}
	return -1;
}

long
SRreqsense(ScsiReq *rp)
{
	uchar cmd[6];
	ScsiReq req;
	long status;

	if(rp->status == Status_SD){
		rp->status = STok;
		return 0;
	}
	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdRsense;
	cmd[4] = sizeof(req.sense);
	memset(&req, 0, sizeof(req));
	if(rp->flags&Fusb)
		req.flags |= Fusb;
	req.fd = rp->fd;
	req.umsc = rp->umsc;
	req.cmd.p = cmd;
	req.cmd.count = sizeof cmd;
	req.data.p = rp->sense;
	req.data.count = sizeof(rp->sense);
	req.data.write = 0;
	status = SRrequest(&req);
	rp->status = req.status;
	return status;
}

long
SRformat(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdFormat;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 6;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRrblimits(ScsiReq *rp, uchar *list)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdRblimits;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = list;
	rp->data.count = 6;
	rp->data.write = 0;
	return SRrequest(rp);
}

static int
dirdevrw(ScsiReq *rp, uchar *cmd, long nbytes)
{
	long n;

	n = nbytes / rp->lbsize;
	if(rp->offset <= Max24off && n <= 256 && (rp->flags & Frw10) == 0){
		PUTBE24(cmd+1, rp->offset);
		cmd[4] = n;
		cmd[5] = 0;
		return 6;
	}
	cmd[0] |= ScmdExtread;
	cmd[1] = 0;
	PUTBELONG(cmd+2, rp->offset);
	cmd[6] = 0;
	cmd[7] = n>>8;
	cmd[8] = n;
	cmd[9] = 0;
	return 10;
}

static int
seqdevrw(ScsiReq *rp, uchar *cmd, long nbytes)
{
	long n;

	/* don't set Cmd1sili; we want the ILI bit instead of a fatal error */
	cmd[1] = rp->flags&Fbfixed? Cmd1fixed: 0;
	n = nbytes / rp->lbsize;
	PUTBE24(cmd+2, n);
	cmd[5] = 0;
	return 6;
}

long
SRread(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[10];
	long n;

	if((nbytes % rp->lbsize) || nbytes > maxiosize){
		rp->status = Status_BADARG;
		return -1;
	}

	/* set up scsi read cmd */
	cmd[0] = ScmdRead;
	if(rp->flags & Fseqdev)
		rp->cmd.count = seqdevrw(rp, cmd, nbytes);
	else
		rp->cmd.count = dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 0;

	/* issue it */
	n = SRrequest(rp);
	if(n != -1){			/* it worked? */
		rp->offset += n / rp->lbsize;
		return n;
	}

	/* request failed; maybe we just read a short record? */
	if (exabyte) {
		fprint(2, "read error\n");
		rp->status = STcheck;
		return n;
	}
	if(rp->status != Status_SD || !(rp->sense[0] & Sd0valid))
		return -1;
	/* compute # of bytes not read */
	n = GETBELONG(rp->sense+3) * rp->lbsize;
	if (debug)
		fprint(2,
	"SRread: request failed with sense data; sense byte count %ld\n",
			n);
	if(!(rp->flags & Fseqdev))
		return -1;

	/* device is a tape or something similar */
	if (rp->sense[2] == Sd2filemark || rp->sense[2] == 0x08 ||
	    rp->sense[2] & Sd2ili && n > 0)
		rp->data.count = nbytes - n;
	else
		return -1;
	n = rp->data.count;
	if (!rp->readblock++ || debug)
		fprint(2, "SRread: tape data count %ld%s\n", n,
			(rp->sense[2] & Sd2ili? " with ILI": ""));
	rp->status = STok;
	rp->offset += n / rp->lbsize;
	return n;
}

long
SRwrite(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[10];
	long n;

	if((nbytes % rp->lbsize) || nbytes > maxiosize){
		rp->status = Status_BADARG;
		return -1;
	}

	/* set up scsi write cmd */
	cmd[0] = ScmdWrite;
	if(rp->flags & Fseqdev)
		rp->cmd.count = seqdevrw(rp, cmd, nbytes);
	else
		rp->cmd.count = dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 1;

	/* issue it */
	if((n = SRrequest(rp)) == -1){
		if (exabyte) {
			fprint(2, "write error\n");
			rp->status = STcheck;
			return n;
		}
		if(rp->status != Status_SD || rp->sense[2] != Sd2eom)
			return -1;
		if(rp->sense[0] & Sd0valid){
			n -= GETBELONG(rp->sense+3) * rp->lbsize;
			rp->data.count = nbytes - n;
		}
		else
			rp->data.count = nbytes;
		n = rp->data.count;
	}
	rp->offset += n / rp->lbsize;
	return n;
}

long
SRseek(ScsiReq *rp, long offset, int type)
{
	uchar cmd[10];

	switch(type){

	case 0:
		break;

	case 1:
		offset += rp->offset;
		if(offset >= 0)
			break;
		/*FALLTHROUGH*/

	default:
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof cmd);
	if(offset <= Max24off && (rp->flags & Frw10) == 0){
		cmd[0] = ScmdSeek;
		PUTBE24(cmd+1, offset & Max24off);
		rp->cmd.count = 6;
	}else{
		cmd[0] = ScmdExtseek;
		PUTBELONG(cmd+2, offset);
		rp->cmd.count = 10;
	}
	rp->cmd.p = cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	SRrequest(rp);
	if(rp->status == STok)
		return rp->offset = offset;
	return -1;
}

long
SRfilemark(ScsiReq *rp, ulong howmany)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdFmark;
	PUTBE24(cmd+2, howmany);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRspace(ScsiReq *rp, uchar code, long howmany)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdSpace;
	cmd[1] = code;
	PUTBE24(cmd+2, howmany);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	/*
	 * what about rp->offset?
	 */
	return SRrequest(rp);
}

long
SRinquiry(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdInq;
	cmd[4] = sizeof rp->inquiry;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	memset(rp->inquiry, 0, sizeof rp->inquiry);
	rp->data.p = rp->inquiry;
	rp->data.count = sizeof rp->inquiry;
	rp->data.write = 0;
	if(SRrequest(rp) >= 0){
		rp->flags |= Finqok;
		return 0;
	}
	rp->flags &= ~Finqok;
	return -1;
}

long
SRmodeselect6(ScsiReq *rp, uchar *list, long nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdMselect6;
	if((rp->flags & Finqok) && (rp->inquiry[2] & 0x07) >= 2)
		cmd[1] = 0x10;
	cmd[4] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRmodeselect10(ScsiReq *rp, uchar *list, long nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof cmd);
	if((rp->flags & Finqok) && (rp->inquiry[2] & 0x07) >= 2)
		cmd[1] = 0x10;
	cmd[0] = ScmdMselect10;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRmodesense6(ScsiReq *rp, uchar page, uchar *list, long nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdMsense6;
	cmd[2] = page;
	cmd[4] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRmodesense10(ScsiReq *rp, uchar page, uchar *list, long nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdMsense10;
	cmd[2] = page;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRstart(ScsiReq *rp, uchar code)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdStart;
	cmd[4] = code;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRrcapacity(ScsiReq *rp, uchar *data)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdRcapacity;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = data;
	rp->data.count = 8;
	rp->data.write = 0;
	return SRrequest(rp);
}

static long
request(int fd, ScsiPtr *cmd, ScsiPtr *data, int *status)
{
	long n, r;
	char buf[16];

	/* this was an experiment but it seems to be a good idea */
	*status = STok;

	/* send SCSI command */
	if(write(fd, cmd->p, cmd->count) != cmd->count){
		fprint(2, "scsireq: write cmd: %r\n");
		*status = Status_SW;
		return -1;
	}

	/* read or write actual data */
	werrstr("");
	if(data->write)
		n = write(fd, data->p, data->count);
	else {
		n = read(fd, data->p, data->count);
		if (n < 0)
			memset(data->p, 0, data->count);
		else if (n < data->count)
			memset(data->p + n, 0, data->count - n);
	}
	if (n != data->count && n <= 0) {
		if (debug)
			fprint(2,
	"request: tried to %s %ld bytes of data for cmd 0x%x but got %r\n",
				(data->write? "write": "read"),
				data->count, cmd->p[0]);
	} else if (n != data->count && (data->write || debug))
		fprint(2, "request: %s %ld of %ld bytes of actual data\n",
			(data->write? "wrote": "read"), n, data->count);

	/* read status */
	buf[0] = '\0';
	r = read(fd, buf, sizeof buf-1);
	if(exabyte && r <= 0 || !exabyte && r < 0){
		fprint(2, "scsireq: read status: %r\n");
		*status = Status_SW;
		return -1;
	}
	if (r >= 0)
		buf[r] = '\0';
	*status = atoi(buf);
	if(n < 0 && (exabyte || *status != STcheck))
		fprint(2, "scsireq: status 0x%2.2uX: data transfer: %r\n",
			*status);
	return n;
}

long
SRrequest(ScsiReq *rp)
{
	long n;
	int status;

retry:
	if(rp->flags&Fusb)
		n = umsrequest(rp->umsc, &rp->cmd, &rp->data, &status);
	else
		n = request(rp->fd, &rp->cmd, &rp->data, &status);
	switch(rp->status = status){

	case STok:
		rp->data.count = n;
		break;

	case STcheck:
		if(rp->cmd.p[0] != ScmdRsense && SRreqsense(rp) != -1)
			rp->status = Status_SD;
		if (exabyte)
			fprint(2, "SRrequest: STcheck, returning -1\n");
		return -1;

	case STbusy:
		sleep(1000);
		goto retry;

	default:
		fprint(2, "status 0x%2.2uX\n", status);
		return -1;
	}
	return n;
}

int
SRclose(ScsiReq *rp)
{
	if((rp->flags & Fopen) == 0){
		rp->status = Status_BADARG;
		return -1;
	}
	close(rp->fd);
	rp->flags = 0;
	return 0;
}

static int
dirdevopen(ScsiReq *rp)
{
	ulong blocks;
	uchar data[8];

	if(SRstart(rp, 1) == -1 || SRrcapacity(rp, data) == -1)
		return -1;
	rp->lbsize = GETBELONG(data+4);
	blocks =     GETBELONG(data);
	/* some newer dev's don't support 6-byte commands */
	if(blocks > Max24off && !force6bytecmds)
		rp->flags |= Frw10;
	return 0;
}

static int
seqdevopen(ScsiReq *rp)
{
	uchar mode[16], limits[6];

	if(SRrblimits(rp, limits) == -1)
		return -1;
	if(limits[1] == 0 && limits[2] == limits[4] && limits[3] == limits[5]){
		rp->flags |= Fbfixed;
		rp->lbsize = limits[4]<<8 | limits[5];
		return 0;
	}
	/*
	 * On some older hardware the optional 10-byte
	 * modeselect command isn't implemented.
	 */
	if (force6bytecmds)
		rp->flags |= Fmode6;
	if(!(rp->flags & Fmode6)){
		/* try 10-byte command first */
		memset(mode, 0, sizeof mode);
		mode[3] = 0x10;		/* device-specific param. */
		mode[7] = 8;		/* block descriptor length */
		/*
		 * exabytes can't handle this, and
		 * modeselect(10) is optional.
		 */
		if(SRmodeselect10(rp, mode, sizeof mode) != -1){
			rp->lbsize = 1;
			return 0;	/* success */
		}
		/* can't do 10-byte commands, back off to 6-byte ones */
		rp->flags |= Fmode6;
	}

	/* 6-byte command */
	memset(mode, 0, sizeof mode);
	mode[2] = 0x10;		/* device-specific param. */
	mode[3] = 8;		/* block descriptor length */
	/*
	 * bsd sez exabytes need this bit (NBE: no busy enable) in
	 * vendor-specific page (0), but so far we haven't needed it.
	mode[12] |= 8;
	 */
	if(SRmodeselect6(rp, mode, 4+8) == -1)
		return -1;
	rp->lbsize = 1;
	return 0;
}

static int
wormdevopen(ScsiReq *rp)
{
	long status;
	uchar list[MaxDirData];

	if (SRstart(rp, 1) == -1 ||
	    (status = SRmodesense10(rp, Allmodepages, list, sizeof list)) == -1)
		return -1;
	/* nbytes = list[0]<<8 | list[1]; */

	/* # of bytes of block descriptors of 8 bytes each; not even 1? */
	if((list[6]<<8 | list[7]) < 8)
		rp->lbsize = 2048;
	else
		/* last 3 bytes of block 0 descriptor */
		rp->lbsize = GETBE24(list+13);
	return status;
}

int
SRopenraw(ScsiReq *rp, char *unit)
{
	char name[128];

	if(rp->flags & Fopen){
		rp->status = Status_BADARG;
		return -1;
	}
	memset(rp, 0, sizeof *rp);
	rp->unit = unit;

	sprint(name, "%s/raw", unit);

	if((rp->fd = open(name, ORDWR)) == -1){
		rp->status = STtimeout;
		return -1;
	}
	rp->flags = Fopen;
	return 0;
}

int
SRopen(ScsiReq *rp, char *unit)
{
	if(SRopenraw(rp, unit) == -1)
		return -1;
	SRready(rp);
	if(SRinquiry(rp) >= 0){
		switch(rp->inquiry[0]){

		default:
			fprint(2, "unknown device type 0x%.2x\n", rp->inquiry[0]);
			rp->status = Status_SW;
			break;

		case 0x00:	/* Direct access (disk) */
		case 0x05:	/* CD-ROM */
		case 0x07:	/* rewriteable MO */
			if(dirdevopen(rp) == -1)
				break;
			return 0;

		case 0x01:	/* Sequential eg: tape */
			rp->flags |= Fseqdev;
			if(seqdevopen(rp) == -1)
				break;
			return 0;

		case 0x02:	/* Printer */
			rp->flags |= Fprintdev;
			return 0;

		case 0x04:	/* Worm */
			rp->flags |= Fwormdev;
			if(wormdevopen(rp) == -1)
				break;
			return 0;

		case 0x08:	/* medium-changer */
			rp->flags |= Fchanger;
			return 0;
		}
	}
	SRclose(rp);
	return -1;
}
