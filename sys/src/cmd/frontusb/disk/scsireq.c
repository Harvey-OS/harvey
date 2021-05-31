/*
 * This is /sys/src/cmd/scuzz/scsireq.c
 * changed to add more debug support, to keep
 * disk compiling without a scuzz that includes these changes.
 * Also, this includes minor tweaks for usb:
 *	we set req.lun/unit to rp->lun/unit in SRreqsense
 *	we set the rp->sense[0] bit Sd0valid in SRreqsense
 * This does not use libdisk to retrieve the scsi error to make
 * user see the diagnostics if we boot with debug enabled.
 *
 * BUGS:
 *	no luns
 *	and incomplete in many other ways
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
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

static char *scmdnames[256] = {
[ScmdTur]	"Tur",
[ScmdRewind]	"Rewind",
[ScmdRsense]	"Rsense",
[ScmdFormat]	"Format",
[ScmdRblimits]	"Rblimits",
[ScmdRead]	"Read",
[ScmdWrite]	"Write",
[ScmdSeek]	"Seek",
[ScmdFmark]	"Fmark",
[ScmdSpace]	"Space",
[ScmdInq]	"Inq",
[ScmdMselect6]	"Mselect6",
[ScmdMselect10]	"Mselect10",
[ScmdMsense6]	"Msense6",
[ScmdMsense10]	"Msense10",
[ScmdStart]	"Start",
[ScmdRcapacity]	"Rcapacity",
[ScmdRcapacity16]	"Rcap16",
[ScmdExtread]	"Extread",
[ScmdExtwrite]	"Extwrite",
[ScmdExtseek]	"Extseek",

[ScmdSynccache]	"Synccache",
[ScmdRTOC]	"RTOC",
[ScmdRdiscinfo]	"Rdiscinfo",
[ScmdRtrackinfo]	"Rtrackinfo",
[ScmdReserve]	"Reserve",
[ScmdBlank]	"Blank",

[ScmdCDpause]	"CDpause",
[ScmdCDstop]	"CDstop",
[ScmdCDplay]	"CDplay",
[ScmdCDload]	"CDload",
[ScmdCDscan]	"CDscan",
[ScmdCDstatus]	"CDstatus",
[Scmdgetconf]	"getconf",
};

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
	req.lun = rp->lun;
	req.unit = rp->unit;
	req.fd = rp->fd;
	req.umsc = rp->umsc;
	req.cmd.p = cmd;
	req.cmd.count = sizeof cmd;
	req.data.p = rp->sense;
	req.data.count = sizeof(rp->sense);
	req.data.write = 0;
	status = SRrequest(&req);
	rp->status = req.status;
	if(status != -1)
		rp->sense[0] |= Sd0valid;
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
	if(rp->offset > 0xFFFFFFFFUL){
		cmd[0] |= ScmdRead16;
		cmd[1] = 0;
		cmd[2] = rp->offset>>56;
		cmd[3] = rp->offset>>48;
		cmd[4] = rp->offset>>40;
		cmd[5] = rp->offset>>32;
		cmd[6] = rp->offset>>24;
		cmd[7] = rp->offset>>16;
		cmd[8] = rp->offset>>8;
		cmd[9] = rp->offset;
		cmd[10] = n>>24;
		cmd[11] = n>>16;
		cmd[12] = n>>8;
		cmd[13] = n;
		cmd[14] = 0;
		cmd[15] = 0;
		return 16;
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

extern int diskdebug;

long
SRread(ScsiReq *rp, void *buf, long nbytes)
{
	uchar cmd[16];
	long n;

	if(rp->lbsize == 0 || (nbytes % rp->lbsize) || nbytes > Maxiosize){
		if(diskdebug)
			if (nbytes % rp->lbsize)
				fprint(2, "disk: i/o size %ld %% %ld != 0\n",
					nbytes, rp->lbsize);
			else
				fprint(2, "disk: i/o size %ld > %d\n",
					nbytes, Maxiosize);
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
	uchar cmd[16];
	long n;

	if(rp->lbsize == 0 || (nbytes % rp->lbsize) || nbytes > Maxiosize){
		if(diskdebug)
			if (nbytes % rp->lbsize)
				fprint(2, "disk: i/o size %ld %% %ld != 0\n",
					nbytes, rp->lbsize);
			else
				fprint(2, "disk: i/o size %ld > %d\n",
					nbytes, Maxiosize);
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
SRseek(ScsiReq *rp, vlong offset, int type)
{
	uchar cmd[16];

	switch(type){

	case 0:
		break;

	case 1:
		offset += rp->offset;
		if(offset >= 0)
			break;
		/*FALLTHROUGH*/

	default:
		if(diskdebug)
			fprint(2, "disk: seek failed\n");
		rp->status = Status_BADARG;
		return -1;
	}
	memset(cmd, 0, sizeof cmd);
	if(offset <= Max24off && (rp->flags & Frw10) == 0){
		cmd[0] = ScmdSeek;
		PUTBE24(cmd+1, offset & Max24off);
		rp->cmd.count = 6;
	}else if(offset <= 0xFFFFFFFFUL) {
		cmd[0] = ScmdExtseek;
		PUTBELONG(cmd+2, offset);
		rp->cmd.count = 10;
	}else {
		cmd[0] = ScmdSeek16;
		cmd[2] = offset>>56;
		cmd[3] = offset>>48;
		cmd[4] = offset>>40;
		cmd[5] = offset>>32;
		cmd[6] = offset>>24;
		cmd[7] = offset>>16;
		cmd[8] = offset>>8;
		cmd[9] = offset;
		rp->cmd.count = 16;
	}
	rp->cmd.p = cmd;
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	SRrequest(rp);
	if(rp->status == STok) {
		rp->offset = offset;
		return offset;
	}
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
	cmd[4] = 36;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	memset(rp->inquiry, 0, sizeof rp->inquiry);
	rp->data.p = rp->inquiry;
	rp->data.count = 36;
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

long
SRrcapacity16(ScsiReq *rp, uchar *data)
{
	uchar cmd[16];
	uint i;

	i = 32;
	memset(cmd, 0, sizeof cmd);
	cmd[0] = ScmdRcapacity16;
	cmd[1] = 0x10;
	cmd[10] = i>>24;
	cmd[11] = i>>16;
	cmd[12] = i>>8;
	cmd[13] = i;

	rp->cmd.p = cmd;
	rp->cmd.count = sizeof cmd;
	rp->data.p = data;
	rp->data.count = i;
	rp->data.write = 0;
	return SRrequest(rp);
}

void
scsidebug(int d)
{
	debug = d;
	if(debug)
		fprint(2, "scsidebug on\n");
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
//	alarm(5*1000);
	if(data->write)
		n = write(fd, data->p, data->count);
	else {
		n = read(fd, data->p, data->count);
		if (n < 0)
			memset(data->p, 0, data->count);
		else if (n < data->count)
			memset(data->p + n, 0, data->count - n);
	}
//	alarm(0);
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

static char*
seprintcmd(char *s, char* e, char *cmd, int count, int args)
{
	uint c;

	if(count < 6)
		return seprint(s, e, "<short cmd>");
	c = cmd[0];
	if(scmdnames[c] != nil)
		s = seprint(s, e, "%s", scmdnames[c]);
	else
		s = seprint(s, e, "cmd:%#02uX", c);
	if(args != 0)
		switch(c){
		case ScmdRsense:
		case ScmdInq:
		case ScmdMselect6:
		case ScmdMsense6:
			s = seprint(s, e, " sz %d", cmd[4]);
			break;
		case ScmdSpace:
			s = seprint(s, e, " code %d", cmd[1]);
			break;
		case ScmdStart:
			s = seprint(s, e, " code %d", cmd[4]);
			break;
	
		}
	return s;
}

static char*
seprintdata(char *s, char *se, uchar *p, int count)
{
	int i;

	if(count == 0)
		return s;
	for(i = 0; i < 20 && i < count; i++)
		s = seprint(s, se, " %02x", p[i]);
	return s;
}

static void
SRdumpReq(ScsiReq *rp)
{
	char buf[128];
	char *s;
	char *se;

	se = buf+sizeof(buf);
	s = seprint(buf, se, "lun %d ", rp->lun);
	s = seprintcmd(s, se, (char*)rp->cmd.p, rp->cmd.count, 1);
	s = seprint(s, se, " [%ld]", rp->data.count);
	if(rp->cmd.write)
		seprintdata(s, se, rp->data.p, rp->data.count);
	fprint(2, "scsi⇒ %s\n", buf);
}

static void
SRdumpRep(ScsiReq *rp)
{
	char buf[128];
	char *s;
	char *se;

	se = buf+sizeof(buf);
	s = seprint(buf, se, "lun %d ", rp->lun);
	s = seprintcmd(s, se, (char*)rp->cmd.p, rp->cmd.count, 0);
	switch(rp->status){
	case STok:
		s = seprint(s, se, " good [%ld] ", rp->data.count);
		if(rp->cmd.write == 0)
			s = seprintdata(s, se, rp->data.p, rp->data.count);
		break;
	case STnomem:
		s = seprint(s, se, " buffer allocation failed");
		break;
	case STharderr:
		s = seprint(s, se, " controller error");
		break;
	case STtimeout:
		s = seprint(s, se, " bus timeout");
		break;
	case STcheck:
		s = seprint(s, se, " check condition");
		break;
	case STcondmet:
		s = seprint(s, se, " condition met/good");
		break;
	case STbusy:
		s = seprint(s, se, " busy");
		break;
	case STintok:
		s = seprint(s, se, " intermediate/good");
		break;
	case STintcondmet:
		s = seprint(s, se, " intermediate/condition met/good");
		break;
	case STresconf:
		s = seprint(s, se, " reservation conflict");
		break;
	case STterminated:
		s = seprint(s, se, " command terminated");
		break;
	case STqfull:
		s = seprint(s, se, " queue full");
		break;
	default:
		s = seprint(s, se, " sts=%#x", rp->status);
	}
	USED(s);
	fprint(2, "scsi← %s\n", buf);
}

static char*
scsierr(ScsiReq *rp)
{
	int ec;

	switch(rp->status){
	case 0:
		return "";
	case Status_SD:
		ec = (rp->sense[12] << 8) | rp->sense[13];
		return scsierrmsg(ec);
	case Status_SW:
		return "software error";
	case Status_BADARG:
		return "bad argument";
	case Status_RO:
		return "device is read only";
	default:
		return "unknown";
	}
}

static void
SRdumpErr(ScsiReq *rp)
{
	char buf[128];
	char *se;

	se = buf+sizeof(buf);
	seprintcmd(buf, se, (char*)rp->cmd.p, rp->cmd.count, 0);
	print("\t%s status: %s\n", buf, scsierr(rp));
}

long
SRrequest(ScsiReq *rp)
{
	long n;
	int status;

retry:
	if(debug)
		SRdumpReq(rp);
	if(rp->flags&Fusb)
		n = umsrequest(rp->umsc, &rp->cmd, &rp->data, &status);
	else
		n = request(rp->fd, &rp->cmd, &rp->data, &status);
	rp->status = status;
	if(status == STok)
		rp->data.count = n;
	if(debug)
		SRdumpRep(rp);
	switch(status){
	case STok:
		break;
	case STcheck:
		if(rp->cmd.p[0] != ScmdRsense && SRreqsense(rp) != -1)
			rp->status = Status_SD;
		if(debug || exabyte)
			SRdumpErr(rp);
		werrstr("%s", scsierr(rp));
		return -1;
	case STbusy:
		sleep(1000);		/* TODO: try a shorter sleep? */
		goto retry;
	default:
		if(debug || exabyte)
			SRdumpErr(rp);
		werrstr("%s", scsierr(rp));
		return -1;
	}
	return n;
}

int
SRclose(ScsiReq *rp)
{
	if((rp->flags & Fopen) == 0){
		if(diskdebug)
			fprint(2, "disk: closing closed file\n");
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
	uvlong blocks;
	uchar data[8+4+20];	/* 16-byte result: lba, blksize, reserved */

	memset(data, 0, sizeof data);
	if(SRstart(rp, 1) == -1 || SRrcapacity(rp, data) == -1)
		return -1;
	rp->lbsize = GETBELONG(data+4);
	blocks =     GETBELONG(data);
	if(debug)
		fprint(2, "disk: dirdevopen: 10-byte logical block size %lud, "
			"# blocks %llud\n", rp->lbsize, blocks);
	if(blocks == 0xffffffff){
		if(SRrcapacity16(rp, data) == -1)
			return -1;
		rp->lbsize = GETBELONG(data + 8);
		blocks = (vlong)GETBELONG(data)<<32 | GETBELONG(data + 4);
		if(debug)
			fprint(2, "disk: dirdevopen: 16-byte logical block size"
				" %lud, # blocks %llud\n", rp->lbsize, blocks);
	}
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
		if(debug)
			fprint(2, "disk: seqdevopen: 10-byte logical block size %lud\n",
				rp->lbsize);
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
	if(debug)
		fprint(2, "disk: wormdevopen: 10-byte logical block size %lud\n",
			rp->lbsize);
	return status;
}

int
SRopenraw(ScsiReq *rp, char *unit)
{
	char name[128];

	if(rp->flags & Fopen){
		if(diskdebug)
			fprint(2, "disk: opening open file\n");
		rp->status = Status_BADARG;
		return -1;
	}
	memset(rp, 0, sizeof *rp);
	rp->unit = unit;

	snprint(name, sizeof name, "%s/raw", unit);
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

		case Devdir:
		case Devcd:
		case Devmo:
			if(dirdevopen(rp) == -1)
				break;
			return 0;

		case Devseq:
			rp->flags |= Fseqdev;
			if(seqdevopen(rp) == -1)
				break;
			return 0;

		case Devprint:
			rp->flags |= Fprintdev;
			return 0;

		case Devworm:
			rp->flags |= Fwormdev;
			if(wormdevopen(rp) == -1)
				break;
			return 0;

		case Devjuke:
			rp->flags |= Fchanger;
			return 0;
		}
	}
	SRclose(rp);
	return -1;
}
