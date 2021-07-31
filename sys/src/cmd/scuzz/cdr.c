#include <u.h>
#include <libc.h>

#include "scsireq.h"

long
SRblank(ScsiReq *rp, uchar type, uchar track)
{
	uchar cmd[12];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdBlank;
	cmd[1] = type;
	cmd[2] = track>>24;
	cmd[3] = track>>16;
	cmd[4] = track>>8;
	cmd[5] = track;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRsynccache(ScsiReq *rp)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdSynccache;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = cmd;
	rp->data.count = 0;
	rp->data.write = 1;
	return SRrequest(rp);
}

long
SRTOC(ScsiReq *rp, void *data, int nbytes, uchar format, uchar track)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRTOC;
	cmd[2] = format;
	cmd[6] = track;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRrdiscinfo(ScsiReq *rp, void *data, int nbytes)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRdiscinfo;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
	rp->data.write = 0;
	return SRrequest(rp);
}

long
SRrtrackinfo(ScsiReq *rp, void *data, int nbytes, int track)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = ScmdRtrackinfo;
	cmd[1] = 0x01;
	cmd[2] = track>>24;
	cmd[3] = track>>16;
	cmd[4] = track>>8;
	cmd[5] = track;
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;
	rp->cmd.p = cmd;
	rp->cmd.count = sizeof(cmd);
	rp->data.p = data;
	rp->data.count = nbytes;
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
	uchar cmd[12];

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
