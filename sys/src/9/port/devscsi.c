#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#define	DPRINT	if(debug)kprint

#define DATASIZE	(64*512)

static Scsi	staticcmd;	/* BUG: should be one per scsi device */

enum
{
	Qdir,
	Qcmd,
	Qdata,
	Qdebug,
};

static Dirtab scsidir[]={
	"cmd",		{Qcmd},		0,	0666,
	"data",		{Qdata},	0,	0666,
	"debug",	{Qdebug},	1,	0666,
};
#define	NSCSI	(sizeof scsidir/sizeof(Dirtab))

extern int	scsidebugs[];
extern int	scsiownid;

static int
scsigen1(Chan *c, long qid, Dir *dp)
{
	if (qid == CHDIR)
		devdir(c, (Qid){qid,0}, ".", 0, eve, 0555, dp);
	else if (qid == 1)
		devdir(c, (Qid){qid,0}, "id", 1, eve, 0666, dp);
	else if (qid&CHDIR) {
		char name[2];
		name[0] = '0'+((qid>>4)&7), name[1] = 0;
		devdir(c, (Qid){qid,0}, name, 0, eve, 0555, dp);
	} else {
		Dirtab *tab = &scsidir[(qid&7)-1];
		devdir(c, (Qid){qid,0}, tab->name, tab->length, eve, tab->perm, dp);
	}
	return 1;
}

static int
scsigeno(Chan *c, Dirtab *tab, long ntab, long s, Dir *dp)
{
	USED(tab, ntab, s);
	return scsigen1(c, c->qid.path, dp);
}

static int
scsigen(Chan *c, Dirtab *tab, long ntab, long s, Dir *dp)
{
	USED(tab, ntab);
	if (c->qid.path == CHDIR) {
		if (0<=s && s<=7)
			return scsigen1(c, CHDIR|0x100|(s<<4), dp);
		else if (s == 8)
			return scsigen1(c, 1, dp);
		else
			return -1;
	}
	if (s >= NSCSI)
		return -1;
	return scsigen1(c, (c->qid.path&~CHDIR)+s+1, dp);
}

void
scsireset(void)
{
	scsibufreset(DATASIZE);
	resetscsi();
}

void
scsiinit(void)
{
	initscsi();
}

Chan *
scsiattach(char *param)
{
	return devattach('S', param);
}

Chan *
scsiclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
scsiwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, scsigen);
}

void
scsistat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, scsigeno);
}

Chan *
scsiopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, scsigeno);
}

void
scsicreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
scsiclose(Chan *c)
{
	Scsi *cmd = &staticcmd;
	
	if((c->qid.path & CHDIR) || c->qid.path==1)
		return;
	if((c->qid.path & 0xf) == Qcmd){
		if(canqlock(cmd) || cmd->pid == u->p->pid){
			cmd->pid = 0;
			qunlock(cmd);
		}
	}
}

long
scsiread(Chan *c, char *a, long n, ulong offset)
{
	Scsi *cmd = &staticcmd;

	if(n == 0)
		return 0;
	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, scsigen);
	if(c->qid.path==1){
		if(offset == 0){
			/*void scsidump(void); scsidump();*/
			*a = scsiownid;
			n = 1;
		}else
			n = 0;
	}else switch(c->qid.path & 0xf){
	case Qcmd:
		if(n < 4)
			error(Ebadarg);
		if(canqlock(cmd)){
			qunlock(cmd);
			error(Egreg);
		}
		if(cmd->pid != u->p->pid)
			error(Egreg);
		n = 4;
		*a++ = 0;
		*a++ = 0;
		*a++ = cmd->status >> 8;
		*a = cmd->status;
		cmd->pid = 0;
		qunlock(cmd);
		break;
	case Qdata:
		if(canqlock(cmd)){
			qunlock(cmd);
			error(Egreg);
		}
		if(cmd->pid != u->p->pid)
			error(Egreg);
		if (n > DATASIZE)
			error(Ebadarg);
		cmd->b = scsibuf();
		cmd->data.base = cmd->b->virt;
		if(waserror()){
			scsifree(cmd->b);
			nexterror();
		}
		cmd->data.lim = cmd->data.base + n;
		cmd->data.ptr = cmd->data.base;
		cmd->save = cmd->data.base;
		scsiexec(cmd, ScsiIn);
		n = cmd->data.ptr - cmd->data.base;
		memmove(a, cmd->data.base, n);
		poperror();
		scsifree(cmd->b);
		break;
	case Qdebug:
		if(offset == 0){
			n=1;
			*a="01"[scsidebugs[(c->qid.path>>4)&7]!=0];
		}else
			n = 0;
		break;
	default:
		panic("scsiread");
	}
	return n;
}

long
scsiwrite(Chan *c, char *a, long n, ulong offset)
{
	Scsi *cmd = &staticcmd;

	if(c->qid.path==1 && n>0){
		if(offset == 0){
			n = 1;
			scsiownid=*a;
			scsireset();
		}else
			n = 0;
	}else switch(c->qid.path & 0xf){
	case Qcmd:
		if(n < 6 || n > sizeof cmd->cmdblk)
			error(Ebadarg);
		qlock(cmd);
		cmd->pid = u->p->pid;
		cmd->cmd.base = cmd->cmdblk;
		memmove(cmd->cmd.base, a, n);
		cmd->cmd.lim = cmd->cmd.base + n;
		cmd->cmd.ptr = cmd->cmd.base;
		cmd->target = (c->qid.path>>4)&7;
		cmd->lun = (a[1]>>5)&7;
		cmd->status = 0xFFFF;
		break;
	case Qdata:
		if(canqlock(cmd)){
			qunlock(cmd);
			error(Egreg);
		}
		if(cmd->pid != u->p->pid)
			error(Egreg);
		if (n > DATASIZE)
			error(Ebadarg);
		cmd->b = scsibuf();
		cmd->data.base = cmd->b->virt;
		if(waserror()){
			scsifree(cmd->b);
			nexterror();
		}
		cmd->data.lim = cmd->data.base + n;
		cmd->data.ptr = cmd->data.base;
		cmd->save = cmd->data.base;
		memmove(cmd->data.base, a, n);
		scsiexec(cmd, ScsiOut);
		n = cmd->data.ptr - cmd->data.base;
		poperror();
		scsifree(cmd->b);
		break;
	case Qdebug:
		if(offset == 0){
			scsidebugs[(c->qid.path>>4)&7] = (*a=='1');
			n = 1;
		}else
			n = 0;
		break;
	default:
		panic("scsiwrite");
	}
	return n;
}

void
scsiremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
scsiwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

Scsi *
scsicmd(int dev, int cmdbyte, Scsibuf *b, long size)
{
	Scsi *cmd = &staticcmd;

	qlock(cmd);
	cmd->target = dev >> 3;
	cmd->lun = dev & 7;
	cmd->cmd.base = cmd->cmdblk;
	cmd->cmd.ptr = cmd->cmd.base;
	memset(cmd->cmdblk, 0, sizeof cmd->cmdblk);
	cmd->cmdblk[0] = cmdbyte;
	switch(cmdbyte >> 5){
	case 0:
		cmd->cmd.lim = &cmd->cmdblk[6];
		break;
	case 1:
		cmd->cmd.lim = &cmd->cmdblk[10];
		break;
	default:
		cmd->cmd.lim = &cmd->cmdblk[12];
		break;
	}
	switch(cmdbyte){
	case ScsiTestunit:
		break;
	case ScsiStartunit:
		cmd->cmdblk[4] = 1;
		break;
	case ScsiModesense:
		cmd->cmdblk[2] = 1;
		/* fall through */
	case ScsiExtsens:
	case ScsiInquiry:
		cmd->cmdblk[4] = size;
		break;
	case ScsiGetcap:
		break;
	}
	cmd->b = b;
	cmd->data.base = b->virt;
	cmd->data.lim = cmd->data.base + size;
	cmd->data.ptr = cmd->data.base;
	cmd->save = cmd->data.base;
	return cmd;
}

int
scsiready(int dev)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiTestunit, scsibuf(), 0);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiOut);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if((status&0xff00) != 0x6000)
		error(Eio);
	return status&0xff;
}

int
scsistartstop(int dev, int cmdbyte)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, cmdbyte, scsibuf(), 0);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiOut);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if((status&0xff00) != 0x6000)
		error(Eio);
	return status&0xff;
}

int
scsisense(int dev, void *p)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiExtsens, scsibuf(), 18);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, 18);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if((status&0xff00) != 0x6000)
		error(Eio);
	return status&0xff;
}

int
scsicap(int dev, void *p)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiGetcap, scsibuf(), 8);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, 8);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if((status&0xff00) != 0x6000)
		error(Eio);
	if(status & 0xFF)
		scsisense(dev, p);
	return status&0xff;
}

int
scsiinquiry(int dev, void *p, int size)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiInquiry, scsibuf(), size);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, size);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if((status&0xff00) != 0x6000)
		error(Eio);
	if(status & 0xFF)
		scsisense(dev, p);
	return status&0xff;
}

int
scsiwp(int dev)
{
/* Device specific
	Scsi *cmd;
	int r, status;

	cmd = scsicmd(dev, ScsiModesense, scsibuf(), 12);
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiIn);
	r = cmd->data.base[2] & 0x80;
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if ((status&0xffff) != 0x6000)
		error(Eio);
	return r;
*/
	USED(dev);
	return 0;
}

int
scsimodesense(int dev, int page, void *p, int size)
{
	Scsi *cmd;
	int status;

	cmd = scsicmd(dev, ScsiModesense, scsibuf(), size);
	cmd->cmdblk[2] = page;
	if(waserror()){
		scsifree(cmd->b);
		qunlock(cmd);
		nexterror();
	}
	status = scsiexec(cmd, ScsiIn);
	memmove(p, cmd->data.base, size);
	poperror();
	scsifree(cmd->b);
	qunlock(cmd);
	if ((status&0xffff) != 0x6000)
		error(Eio);
	return status&0xff;
}

int
scsibread(int dev, Scsibuf *b, long n, long blocksize, long blockno)
{
	Scsi *cmd;

	cmd = scsicmd(dev, ScsiRead, b, n*blocksize);
	if(waserror()){
		qunlock(cmd);
		nexterror();
	}
	cmd->cmdblk[1] = blockno >> 16;
	cmd->cmdblk[2] = blockno >> 8;
	cmd->cmdblk[3] = blockno;
	cmd->cmdblk[4] = n;
	scsiexec(cmd, ScsiIn);
	n = cmd->data.ptr - cmd->data.base;
	poperror();
	qunlock(cmd);
	return n;
}

int
scsibwrite(int dev, Scsibuf *b, long n, long blocksize, long blockno)
{
	Scsi *cmd;

	cmd = scsicmd(dev, ScsiWrite, b, n*blocksize);
	if(waserror()){
		qunlock(cmd);
		nexterror();
	}
	cmd->cmdblk[1] = blockno >> 16;
	cmd->cmdblk[2] = blockno >> 8;
	cmd->cmdblk[3] = blockno;
	cmd->cmdblk[4] = n;
	scsiexec(cmd, ScsiOut);
	n = cmd->data.ptr - cmd->data.base;
	poperror();
	qunlock(cmd);
	return n;
}
