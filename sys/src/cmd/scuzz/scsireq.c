#include <u.h>
#include <libc.h>
/*
 * BUGS:
 *	no luns
 *	and incomplete in many other ways
 */
#include "scsireq.h"

long
SRready(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRrewind(ScsiReq *rp)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRewind;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
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
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRsense;
	cmd[4] = sizeof(req.sense);
	memset(&req, 0, sizeof(req));
	req.fd = rp->fd;
	req.cmd.p = cmd;
	req.cmd.count = sizeof(cmd);
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

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFormat;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 6;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRrblimits(ScsiReq *rp, uchar *list)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRblimits;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = 6;
	rp->data.write = 0;
	return SRrequest(rp);
}

static int
dirdevrw(ScsiReq *rp, uchar *cmd, long nbytes)
{
	long n;

	n = nbytes/rp->lbsize;
	if(rp->offset <= 0x1fffff && n <= 256 && (rp->flags & Frw10) == 0){
		cmd[1] = rp->offset>>16;
		cmd[2] = rp->offset>>8;
		cmd[3] = rp->offset;
		cmd[4] = n;
		cmd[5] = 0;
		return 6;
	}
	cmd[0] |= ScmdExtread;
	cmd[1] = 0;
	cmd[2] = rp->offset>>24;
	cmd[3] = rp->offset>>16;
	cmd[4] = rp->offset>>8;
	cmd[5] = rp->offset;
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

	cmd[1] = rp->flags&Fbfixed? 0x01: 0x00;
	n = nbytes/rp->lbsize;
	cmd[2] = n>>16;
	cmd[3] = n>>8;
	cmd[4] = n;
	cmd[5] = 0;
	return 6;
}

long
SRread(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[10];
	long n;

	if((nbytes % rp->lbsize) || nbytes > MaxIOsize){
		rp->status = Status_BADARG;
		return -1;
	}
	cmd[0] = ScmdRead;
	if(rp->flags & Fseqdev)
		rp->cmd.count = seqdevrw(rp, cmd, nbytes);
	else
		rp->cmd.count = dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 0;
	if((n = SRrequest(rp)) == -1){
		if(rp->status != Status_SD || (rp->sense[0] & 0x80) == 0)
			return -1;
		n = ((rp->sense[3]<<24)
		   | (rp->sense[4]<<16)
		   | (rp->sense[5]<<8)
		   | rp->sense[6])
		   * rp->lbsize;
		if(!(rp->flags & Fseqdev))
			return -1;
		if(rp->sense[2] == 0x80 || rp->sense[2] == 0x08)
			rp->data.count = nbytes - n;
		else if(rp->sense[2] == 0x20 && n > 0)
			rp->data.count = nbytes - n;
		else
			return -1;
		n = rp->data.count;
		rp->status = STok;
	}
	rp->offset += n/rp->lbsize;
	return n;
}

long
SRwrite(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[10];
	long n;

	if((nbytes % rp->lbsize) || nbytes > MaxIOsize){
		rp->status = Status_BADARG;
		return -1;
	}
	cmd[0] = ScmdWrite;
	if(rp->flags & Fseqdev)
		rp->cmd.count = seqdevrw(rp, cmd, nbytes);
	else
		rp->cmd.count = dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 1;
	if((n = SRrequest(rp)) == -1){
		if(rp->status != Status_SD || rp->sense[2] != 0x40)
			return -1;
		if(rp->sense[0] & 0x80){
			n -= ((rp->sense[3]<<24)
			    | (rp->sense[4]<<16)
			    | (rp->sense[5]<<8)
			    | rp->sense[6])
			    * rp->lbsize;
			rp->data.count = nbytes - n;
		}
		else 
			rp->data.count = nbytes;
		n = rp->data.count;
	}
	rp->offset += n/rp->lbsize;
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
	if(offset <= 0x1fffff && (rp->flags & Frw10) == 0){
		cmd[0] = ScmdSeek;
		cmd[1] = (offset>>16) & 0x1F;
		cmd[2] = offset>>8;
		cmd[3] = offset;
		cmd[4] = 0;
		cmd[5] = 0;
		rp->cmd.count = 6;
	}else{
		cmd[0] = ScmdExtseek;
		cmd[1] = 0;
		cmd[2] = offset>>24;
		cmd[3] = offset>>16;
		cmd[4] = offset>>8;
		cmd[5] = offset;
		cmd[6] = 0;
		cmd[7] = 0;
		cmd[8] = 0;
		cmd[9] = 0;
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

	cmd[0] = ScmdFmark;
	cmd[1] = 0;
	cmd[2] = howmany>>16;
	cmd[3] = howmany>>8;
	cmd[4] = howmany;
	cmd[5] = 0;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRspace(ScsiReq *rp, uchar code, long howmany)
{
	uchar cmd[6];

	cmd[0] = ScmdSpace;
	cmd[1] = code;
	cmd[2] = howmany>>16;
	cmd[3] = howmany>>8;
	cmd[4] = howmany;
	cmd[5] = 0;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
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

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdInq;
	cmd[4] = sizeof(rp->inquiry);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = rp->inquiry;
	rp->data.count = sizeof(rp->inquiry);
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

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMselect6;
	if((rp->flags & Finqok) && (rp->inquiry[2] & 0x07) >= 2)
		cmd[1] = 0x10;
	cmd[4] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRmodeselect10(ScsiReq *rp, uchar *list, long nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	if((rp->flags & Finqok) && (rp->inquiry[2] & 0x07) >= 2)
		cmd[1] = 0x10;
	cmd[0] = ScmdMselect10;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRmodesense6(ScsiReq *rp, uchar page, uchar *list, long nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMsense6;
	cmd[2] = page;
	cmd[4] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRmodesense10(ScsiReq *rp, uchar page, uchar *list, long nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMsense10;
	cmd[2] = page;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRstart(ScsiReq *rp, uchar code)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdStart;
	cmd[4] = code;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRrcapacity(ScsiReq *rp, uchar *data)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRcapacity;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = 8;
	rp->data.write = 0;
	return SRrequest(rp);
}

static long
request(int fd, ScsiPtr *cmd, ScsiPtr *data, int *status)
{
	long n;
	char buf[16];

	if(write(fd, cmd->p, cmd->count) != cmd->count){
		fprint(2, "scsireq: write cmd: %r\n");
		*status = Status_SW;
		return -1;
	}
	if(data->write)
		n = write(fd, data->p, data->count);
	else
		n = read(fd, data->p, data->count);
	if(read(fd, buf, sizeof(buf)) < 0){
		fprint(2, "scsireq: read status: %r\n");
		*status = Status_SW;
		return -1;
	}
	buf[sizeof(buf)-1] = '\0';
	*status = atoi(buf);
	if(n < 0 && *status != STcheck)
		fprint(2, "scsireq: status 0x%2.2uX: data transfer: %r\n", *status);
	return n;
}

long
SRrequest(ScsiReq *rp)
{
	long n;
	int status;

retry:
	n = request(rp->fd, &rp->cmd, &rp->data, &status);
	switch(rp->status = status){

	case STok:
		rp->data.count = n;
		break;

	case STcheck:
		if(rp->cmd.p[0] != ScmdRsense && SRreqsense(rp) != -1)
			rp->status = Status_SD;
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
	rp->lbsize = (data[4]<<28)|(data[5]<<16)|(data[6]<<8)|data[7];
	blocks = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|data[3];
	if(blocks > 0x1fffff)
		rp->flags |= Frw10;		/* some newer devices don't support 6-byte commands */
	return 0;
}

static int
seqdevopen(ScsiReq *rp)
{
	uchar mode[16], limits[6];

	if(SRrblimits(rp, limits) == -1)
		return -1;
	if(limits[1] || limits[2] != limits[4] || limits[3] != limits[5]){
		/*
		 * On some older hardware the optional 10-byte
		 * modeselect command isn't implemented.
		 */
		if(!(rp->flags & Fmode6)){
			memset(mode, 0, sizeof(mode));
			mode[3] = 0x10;
			mode[7] = 8;
			if(SRmodeselect10(rp, mode, sizeof(mode)) != -1){
				rp->lbsize = 1;
				return 0;
			}
			rp->flags |= Fmode6;
		}

		memset(mode, 0, sizeof(mode));
		mode[2] = 0x10;
		mode[3] = 8;
		if(SRmodeselect6(rp, mode, 4+8) == -1)
			return -1;
		rp->lbsize = 1;
	}
	else{
		rp->flags |= Fbfixed;
		rp->lbsize = (limits[4]<<8)|limits[5];
	}
	return 0;
}

static int
wormdevopen(ScsiReq *rp)
{
	uchar list[MaxDirData];
	long status;

	if(SRstart(rp, 1) == -1)
		return -1;
	if((status = SRmodesense10(rp, 0x3F, list, sizeof(list))) == -1)
		return -1;
	if(((list[6]<<8)|list[3]) < 8)
		rp->lbsize = 2048;
	else
		rp->lbsize = (list[13]<<8)|(list[14]<<8)|list[15];
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
	memset(rp, 0, sizeof(*rp));
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
