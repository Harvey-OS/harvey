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
		rp->status = Status_OK;
		return 0;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRsense;
	cmd[4] = sizeof(req.sense);
	memset(&req, 0, sizeof(req));
	req.cmd.fd = rp->cmd.fd;
	req.data.fd = rp->data.fd;
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

static void
dirdevrw(ScsiReq *rp, uchar *cmd, long nbytes)
{
	long n;

	n = rp->offset;
	cmd[1] = (n>>16) & 0x0F;
	cmd[2] = n>>8;
	cmd[3] = n;
	cmd[4] = nbytes/rp->lbsize;
	cmd[5] = 0;
}

static void
seqdevrw(ScsiReq *rp, uchar *cmd, long nbytes)
{
	long n;

	cmd[1] = 0x00;
	n = nbytes/rp->lbsize;
	cmd[2] = n>>16;
	cmd[3] = n>>8;
	cmd[4] = n;
	cmd[5] = 0;
}

long
SRread(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[6];
	long n;

	if((nbytes % rp->lbsize) || nbytes > MaxIOsize){
		rp->status = Status_BADARG;
		return -1;
	}
	cmd[0] = ScmdRead;
	if(rp->flags & Fseqdev)
		seqdevrw(rp, cmd, nbytes);
	else
		dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
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
		if(rp->sense[2] == 0x80 || rp->sense[2] == 0x08)
			rp->data.count = nbytes - n;
		else if(rp->sense[2] == 0x20 && n > 0)
			rp->data.count = nbytes - n;
		else
			return -1;
		n = rp->data.count;
		rp->status = Status_OK;
	}
	rp->offset += n/rp->lbsize;
	return n;
}

long
SRwrite(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[6];
	long n;

	if((nbytes % rp->lbsize) || nbytes > MaxIOsize){
		rp->status = Status_BADARG;
		return -1;
	}
	cmd[0] = ScmdWrite;
	if(rp->flags & Fseqdev)
		seqdevrw(rp, cmd, nbytes);
	else
		dirdevrw(rp, cmd, nbytes);
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
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
	uchar cmd[6];

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
	cmd[0] = ScmdSeek;
	cmd[1] = (offset>>16) & 0x0F;
	cmd[2] = offset>>8;
	cmd[3] = offset;
	cmd[4] = 0;
	cmd[5] = 0;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	SRrequest(rp);
	if(rp->status == Status_OK)
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
	return SRrequest(rp);
}

long
SRmodeselect(ScsiReq *rp, uchar *list, long nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMselect;
	cmd[4] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = list;
	rp->data.count = nbytes;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRmodesense(ScsiReq *rp, uchar page, uchar *list, long nbytes)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMsense;
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
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRcapacity;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = 8;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRflushcache(ScsiReq *rp)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFlushcache;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRrdiscinfo(ScsiReq *rp, void *data, uchar ses, uchar track)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRdiscinfo;
	cmd[6] = track;
	cmd[7] = 0;
	cmd[8] = MaxDirData;
	if(ses)
		cmd[9] = 0x40;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = MaxDirData;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRfwaddr(ScsiReq *rp, uchar track, uchar mode, uchar npa, uchar *data)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFwaddr;
	cmd[2] = track;
	cmd[3] = mode;
	cmd[7] = npa;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = MaxDirData;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRtreserve(ScsiReq *rp, long nbytes)
{
	uchar cmd[10];
	long n;

	if((nbytes % rp->lbsize)){
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTreserve;
	n = nbytes/rp->lbsize;
	cmd[5] = n>>24;
	cmd[6] = n>>16;
	cmd[7] = n>>8;
	cmd[8] = n;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRtinfo(ScsiReq *rp, uchar track, uchar *data)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTinfo;
	cmd[5] = track;
	cmd[8] = MaxDirData;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = MaxDirData;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRwtrack(ScsiReq *rp, void *buf, long nbytes, uchar track, uchar mode)
{
	uchar cmd[10];
	long m, n;

	if((nbytes % rp->lbsize) || nbytes > MaxIOsize){
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdTwrite;
	cmd[5] = track;
	cmd[6] = mode;
	n = nbytes/rp->lbsize;
	cmd[7] = n>>8;
	cmd[8] = n;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = buf;
	rp->data.count = nbytes;
	rp->data.write = 1;
	m = SRrequest(rp);
	if(m < 0)
		return -1;
	rp->offset += n;
	return m;
}

long
SRmload(ScsiReq *rp, uchar code)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdMload;
	cmd[8] = code;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRfixation(ScsiReq *rp, uchar type)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdFixation;
	cmd[8] = type;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

static long
request(ScsiPtr *cmd, ScsiPtr *data, uchar status[4])
{
	long n;

	if(write(cmd->fd, cmd->p, cmd->count) != cmd->count){
		fprint(2, "scsireq: write cmd: %r\n");
		return -1;
	}
	if(data->write)
		n = write(data->fd, data->p, data->count);
	else
		n = read(data->fd, data->p, data->count);
	if(read(cmd->fd, status, sizeof(status)) != sizeof(status)){
		fprint(2, "scsireq: read status: %r\n");
		return -1;
	}
	return n;
}

long
SRrequest(ScsiReq *rp)
{
	long n;
	uchar status[4];

retry:
	if((n = request(&rp->cmd, &rp->data, status)) == -1){
		rp->status = Status_SW;
		return -1;
	}
	if(status[2] != 0x60){
		rp->status = Status_HW;
		return -1;
	}
	switch(rp->status = status[3]){

	case Status_OK:
		rp->data.count = n;
		break;

	case Status_CC:
		if(rp->cmd.p[0] != ScmdRsense && SRreqsense(rp) != -1)
			rp->status = Status_SD;
		return -1;

	case Status_BUSY:
		sleep(1000);
		goto retry;
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
	close(rp->cmd.fd);
	close(rp->data.fd);
	rp->flags = 0;
	return 0;
}

static int
dirdevopen(ScsiReq *rp)
{
	uchar data[8];

	if(SRstart(rp, 1) == -1 || SRrcapacity(rp, data) == -1)
		return -1;
	rp->lbsize = (data[4]<<28)|(data[5]<<16)|(data[6]<<8)|data[7];
	return 0;
}

static int
seqdevopen(ScsiReq *rp)
{
	uchar mode[12], limits[6];

	if(SRrblimits(rp, limits) == -1)
		return -1;
	if(limits[1] || limits[2] != limits[4] || limits[3] != limits[5]){
		memset(mode, 0, sizeof(mode));
		mode[2] = 0x10;
		mode[3] = 8;
		if(SRmodeselect(rp, mode, sizeof(mode)) == -1)
			return -1;
		rp->lbsize = 1;
	}
	else
		rp->lbsize = (limits[4]<<8)|limits[5];
	return 0;
}

static int
wormdevopen(ScsiReq *rp)
{
	uchar list[MaxDirData];
	long status;

	if(SRstart(rp, 1) == -1)
		return -1;
	if((status = SRmodesense(rp, 0x3F, list, sizeof(list))) == -1)
		return -1;
	if(list[3] < 8)
		rp->lbsize = 2048;
	else
		rp->lbsize = (list[10]<<8)|list[11];
	return status;
}

int
SRopenraw(ScsiReq *rp, int id)
{
	char name[128];

	if((rp->flags & Fopen) || id >= NTargetID || id == CtlrID){
		rp->status = Status_BADARG;
		return -1;
	}
	memset(rp, 0, sizeof(*rp));
	rp->id = id;
	sprint(name, "#S/%d/cmd", id);
	if((rp->cmd.fd = open(name, ORDWR)) == -1){
		rp->status = Status_NX;
		return -1;
	}
	sprint(name, "#S/%d/data", id);
	if((rp->data.fd = open(name, ORDWR)) == -1){
		close(rp->cmd.fd);
		rp->status = Status_NX;
		return -1;
	}
	rp->flags = Fopen;
	return 0;
}

int
SRopen(ScsiReq *rp, int id)
{
	if(SRopenraw(rp, id) == -1)
		return -1;
	SRready(rp);
	if(SRinquiry(rp) >= 0){
		switch(rp->inquiry[0]){

		default:
			rp->status = Status_NX;
			break;

		case 0x00:	/* Direct access (disk) */
		case 0x05:	/* CD-rom */
			if(dirdevopen(rp) == -1)
				break;
			return 0;

		case 0x01:	/* Sequential eg: tape */
			rp->flags |= Fseqdev;
			if(seqdevopen(rp) == -1)
				break;
			return 0;

		case 0x04:	/* Worm */
			rp->flags |= Fwormdev;
			if(wormdevopen(rp) == -1)
				break;
			return 0;
		}
	}
	SRclose(rp);
	return -1;
}
