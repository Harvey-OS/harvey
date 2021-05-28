#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <regexp.h>
#include <bio.h>
#include "debug.h"
#include "tap.h"
#include "chain.h"
#include "jtag.h"
#include "icert.h"
#include "mmu.h"
#include "mpsse.h"
#include "/sys/src/9/kw/arm.h"
#include "/arm/include/ureg.h"
#include "lebo.h"

/*
 * Like rdbfs + jtag icert control for arm cores.
 * Memory just goes through in arm order, registers get translated
 * to host order and back after.
 */

typedef struct Fid Fid;
typedef struct Fs Fs;
enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nfidhash	= 32,

	/*
	 * qids
	 */
	Qroot	= 1,
	Qctl,
	Qnote,
	Qmem,
			/* copied from rdbfs */
	Qkregs,
	Qfpregs,
	Qproc,
	Qregs,
	Qtext,
	Qstatus,
};

static int		textfd;
static char*	textfile = "/arm/s9plug";


struct Fid
{
	Lock;
	Fid	*next;
	Fid	**last;
	uint	fid;
	int	ref;			/* number of fcalls using the fid */
	int	attached;		/* fid has beed attached or cloned and not clunked */

	int	open;
	Qid	qid;
};

static int			dostat(int, uchar*, int);
static void*		emalloc(uint);
static void			fatal(char*, ...);
static void			usage(void);

struct Fs
{
	Lock;				/* for fids */

	Fid		*hash[Nfidhash];
	uchar	statbuf[1024];	/* plenty big enough */
	JMedium *jmed;
};

static	void		fsrun(Fs*, int);
static	Fid*		getfid(Fs*, uint);
static	Fid*		mkfid(Fs*, uint);
static	void		putfid(Fs*, Fid*);
static	char*	fsversion(Fs*, Fcall*);
static	char*	fsauth(Fs*, Fcall*);
static	char*	fsattach(Fs*, Fcall*);
static	char*	fswalk(Fs*, Fcall*);
static	char*	fsopen(Fs*, Fcall*);
static	char*	fscreate(Fs*, Fcall*);
static	char*	fsread(Fs*, Fcall*);
static	char*	fswrite(Fs*, Fcall*);
static	char*	fsclunk(Fs*, Fcall*);
static	char*	fsremove(Fs*, Fcall*);
static	char*	fsstat(Fs*, Fcall*);
static	char*	fswstat(Fs*, Fcall*);

static char	*(*fcalls[])(Fs*, Fcall*) =
{
	[Tversion]		fsversion,
	[Tattach]		fsattach,
	[Tauth]		fsauth,
	[Twalk]		fswalk,
	[Topen]		fsopen,
	[Tcreate]		fscreate,
	[Tread]		fsread,
	[Twrite]		fswrite,
	[Tclunk]		fsclunk,
	[Tremove]	fsremove,
	[Tstat]		fsstat,
	[Twstat]		fswstat
};

static char	Eperm[]		=	"permission denied";
static char	Enotdir[]		=	"not a directory";
static char	Enotexist[]	=	"file does not exist";
static char	Eisopen[]		= 	"file already open for I/O";
static char	Einuse[]		=	"fid is already in use";
static char	Enofid[]		= 	"no such fid";
static char	Enotopen[]	=	"file is not open";
static char	Ebadcmd[]	=	"error in jtag operation";

static ArmCtxt ctxt;
static Fs		fs;
static int		messagesize = 8192+IOHDRSZ;

static int
cmdsetdebug(uchar *deb)
{
	int i;

	memset(debug, 0, 255);
	for(i = 0; i < strlen((char *)deb); i++){
		debug[deb[i]]++;
	}
	return 0;
}



void
main(int argc, char **argv)
{
	char buf[12], *mnt, *srv, *mbname;
	int fd, p[2], mb;
	uchar *deb;
	
	deb = nil;
	mb = Sheeva;
	mbname = nil;
	mnt = "/n/jtagfs";
	srv = nil;
	ARGBEGIN{
		case 'b':
			mbname = EARGF(usage());
			break;
		case 's':
			srv = ARGF();
			mnt = nil;
			break;
		case 'm':
			mnt = ARGF();
			break;
		case 'd':
			deb = (uchar *)EARGF(usage());
			break;
		case 't':
			textfile = EARGF(usage());
			break;
	}ARGEND

	fmtinstall('F', fcallfmt);
	if(deb  != nil)
		cmdsetdebug(deb);
	if(argc != 1)
		usage();
	print("jtagfs: %s\n", argv[0]);
	fd = open(argv[0], ORDWR);
	if(fd < 0)
		fatal("can't open jtag file %s: %r", argv[0]);

	if(mbname == nil)
		mb = Sheeva;
	else if(strncmp(mbname, "sheeva", 6) == 0)
		mb = Sheeva;
	else if(strncmp(mbname, "gurudisp", 6) == 0)
		mb = GuruDisp;
	fs.jmed = initmpsse(fd, mb);
	if(fs.jmed == nil)
		fatal("jtag initialization %r");
	
	if(pipe(p) < 0)
		fatal("pipe failed");

	switch(rfork(RFPROC|RFMEM|RFNOTEG|RFNAMEG)){
	case 0:
		fsrun(&fs, p[0]);
		exits(nil);
	case -1:	
		fatal("fork failed");
	}

	if(mnt == nil){
		if(srv == nil)
			usage();
		fd = create(srv, OWRITE, 0666);
		if(fd < 0){
			remove(srv);
			fd = create(srv, OWRITE, 0666);
			if(fd < 0){
				close(p[1]);
				fatal("create of %s failed", srv);
			}
		}
		sprint(buf, "%d", p[1]);
		if(write(fd, buf, strlen(buf)) < 0){
			close(p[1]);
			fatal("writing %s", srv);
		}
		close(p[1]);
		exits(nil);
	}

	if(mount(p[1], -1, mnt, MREPL, "") < 0){
		close(p[1]);
		fatal("mount failed");
	}
	close(p[1]);
	exits(nil);
}

static void
fsrun(Fs *fs, int fd)
{
	Fcall rpc;
	char *err;
	uchar *buf;
	int n;

	buf = emalloc(messagesize);
	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read9pmsg(fd, buf, messagesize);
		if(n == 0)
			continue;
		if(n < 0)
			fatal("mount read");

		rpc.data = (char*)buf + IOHDRSZ;
		if(convM2S(buf, n, &rpc) == 0)
			continue;
		// fprint(2, "recv: %F\n", &rpc);


		/*
		 * TODO: flushes are broken
		 */
		if(rpc.type == Tflush)
			continue;
		else if(rpc.type >= Tmax || !fcalls[rpc.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rpc.type])(fs, &rpc);
		if(err){
			rpc.type = Rerror;
			rpc.ename = err;
		}
		else
			rpc.type++;
		n = convS2M(&rpc, buf, messagesize);
		// fprint(2, "send: %F\n", &rpc);
		if(write(fd, buf, n) != n)
			fatal("mount write");
	}
}

static Fid*
mkfid(Fs *fs, uint fid)
{
	Fid *f;
	int h;

	h = fid % Nfidhash;
	for(f = fs->hash[h]; f; f = f->next){
		if(f->fid == fid)
			return nil;
	}

	f = emalloc(sizeof *f);
	f->next = fs->hash[h];
	if(f->next != nil)
		f->next->last = &f->next;
	f->last = &fs->hash[h];
	fs->hash[h] = f;

	f->fid = fid;
	f->ref = 1;
	f->attached = 1;
	f->open = 0;
	return f;
}

static Fid*
getfid(Fs *fs, uint fid)
{
	Fid *f;
	int h;

	h = fid % Nfidhash;
	for(f = fs->hash[h]; f; f = f->next){
		if(f->fid == fid){
			if(f->attached == 0)
				break;
			f->ref++;
			return f;
		}
	}
	return nil;
}

static void
putfid(Fs *, Fid *f)
{
	f->ref--;
	if(f->ref == 0 && f->attached == 0){
		*f->last = f->next;
		if(f->next != nil)
			f->next->last = f->last;
		free(f);
	}
}

static char*
fsversion(Fs *, Fcall *rpc)
{
	if(rpc->msize < 256)
		return "version: message size too small";
	if(rpc->msize > messagesize)
		rpc->msize = messagesize;
	messagesize = rpc->msize;
	if(strncmp(rpc->version, "9P2000", 6) != 0)
		return "unrecognized 9P version";
	rpc->version = "9P2000";
	return nil;
}

static char*
fsauth(Fs *, Fcall *)
{
	return "searchfs: authentication not required";
}

static char*
fsattach(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = mkfid(fs, rpc->fid);
	if(f == nil)
		return Einuse;
	f->open = 0;
	f->qid.type = QTDIR;
	f->qid.path = Qroot;
	f->qid.vers = 0;
	rpc->qid = f->qid;
	putfid(fs, f);
	return nil;
}

typedef struct DEntry DEntry;
struct DEntry {
	uvlong path;
	char *name;
};

DEntry entries[] = {
	{Qctl,	"ctl"},
	{Qmem,	"mem"},
	{Qkregs,	"kregs"},
	{Qfpregs,	"fpregs"},
	{Qproc,	"proc"},
	{Qregs,	"regs"},
	{Qtext,	"text"},
	{Qstatus,	"status"},
	{Qnote,	"note"},
	{~0,	nil},
};

static uvlong
nm2qpath(char *name)
{
	int i;
	
	for(i = 0; entries[i].name != nil; i++)
		if(strcmp(name, entries[i].name) == 0)
			return entries[i].path;

	return ~0;
}

static char*
fswalk(Fs *fs, Fcall *rpc)
{
	Fid *f, *nf;
	int nqid, nwname, type;
	char *err, *name;
	ulong path, fpath;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	nf = nil;
	if(rpc->fid != rpc->newfid){
		nf = mkfid(fs, rpc->newfid);
		if(nf == nil){
			putfid(fs, f);
			return Einuse;
		}
		nf->qid = f->qid;
		putfid(fs, f);
		f = nf;	/* walk f */
	}

	err = nil;
	path = f->qid.path;
	nwname = rpc->nwname;
	for(nqid=0; nqid<nwname; nqid++){
		if(path != Qroot){
			err = Enotdir;
			break;
		}
		name = rpc->wname[nqid];
		/* BUG this is a kludge, rewrite */
		fpath = nm2qpath(name);
		if(fpath != ~0){
			type = QTFILE;
			path = fpath;
		}
		else	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			type = QTDIR;
		else {
			err = Enotexist;
			break;
		}

		rpc->wqid[nqid] = (Qid){path, 0, type};
	}

	if(nwname > 0){
		if(nf != nil && nqid < nwname)
			nf->attached = 0;
		if(nqid == nwname)
			f->qid = rpc->wqid[nqid-1];
	}

	putfid(fs, f);
	rpc->nwqid = nqid;
	f->open = 0;
	return err;
}

static char *
fsopen(Fs *fs, Fcall *rpc)
{
	Fid *f;
	int mode;
	char buf[512];

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(f->open){
		putfid(fs, f);
		return Eisopen;
	}
	mode = rpc->mode & OPERM;
	if(mode == OEXEC
	|| f->qid.path == Qroot && (mode == OWRITE || mode == ORDWR)){
		putfid(fs, f);
		return Eperm;
	}
	if(f->qid.path == Qtext){
		close(textfd);
		textfd = open(textfile, OREAD);
		if(textfd < 0) {
			snprint(buf, sizeof buf, "text: %r");
			putfid(fs, f);
			return "opening text file";
		}
	}
	f->open = 1;
	rpc->qid = f->qid;
	rpc->iounit = messagesize-IOHDRSZ;
	putfid(fs, f);
	return nil;
}

static char *
fscreate(Fs *, Fcall *)
{
	return Eperm;
}

static int
readbuf(Fcall *rpc, void *s, long n)
{
	int count;
	count = rpc->count;
	if(rpc->offset >= n){
		count = 0;
	}
	if(rpc->offset+count > n)
		count = n - rpc->offset;
	memmove(rpc->data, (char*)s+rpc->offset, count);
	rpc->count = count;
	return count;
}

enum{
	Maxctl = 4*1024,
};

static char ctlread[Maxctl];

static int
readctl(Fs *fs, Fcall *rpc)
{
	char *e;
	int ssz;

	ssz = Maxctl;
	ctxt.debug = icedebugstate(fs->jmed);

	if(ctxt.debug == 0){
		e = seprint(ctlread, ctlread+Maxctl, "Arm is in debug: %d", ctxt.debug);
		ssz = readbuf(rpc, ctlread, e - ctlread);
		return ssz;
	}
	e = armsprctxt(&ctxt, ctlread, ssz);
	if(e >= ctlread + Maxctl)
		return e - ctlread;
	ssz = Maxctl - (ctlread - e);
	e = printmmuregs(&ctxt, e, ssz);

	ssz = readbuf(rpc, ctlread, e - ctlread);
	return ssz;

}

static int
readbytes(JMedium *jmed, u32int startaddr, u32int nbytes, u32int dataoff, Fcall *rpc)
{
	u32int i, data;
	int nb, res;

	nb = 0;
	for(i = startaddr; i < startaddr+nbytes; i++){
		res = armrdmemwd(jmed, i, &data, 1);
		if(res < 0){
			fprint(2, "Error reading [%#8.8ux]\n",
				i);
			werrstr("read error");
			return -1;
			break;
		}
		nb += res;
		rpc->data[dataoff] = (char)data;
		dprint(Dfs, "[B%#8.8ux] = %#2.2ux \n",
				i, data);
	}
	return nb;

}

/* 
 *	BUG: This is horrifyingly slow, could be made much (10x)
 *	faster using load multiple/store multiple
 * 	both on memory and while reading back the registers.
 *	but it doesn't matter for acid (normally it reads words
 *	or byte long chunks). There may also be another way
 *	where you can inject instructions without waiting for
 *	debug mode on the way back. I haven't been able to
 *	make it work.
 */
static int
readmem(Fs *fs, Fcall *rpc)
{
	u32int count, i, data, addr, prenb, nb, postnb, st;
	int res;

	count = (u32int)rpc->count;
	
	addr = (u32int)rpc->offset;
	dprint(Dfs, "[%#8.8ux, %ud] =?\n", 
				addr, count);

	prenb = 0;
	nb = 0;
	/* The start is not aligned */
	if(addr & 0x3U){
		prenb = sizeof(u32int) - (addr & 0x3U);
		res = readbytes(fs->jmed, addr, prenb, 0, rpc);
		if(res < 0){
			werrstr("read error");
			return -1;
		}
		nb += res;
		dprint(Dfs, "readmem: aligned now\n");
	}
	for(i = prenb; i/4 < (count-prenb)/sizeof(u32int); i += sizeof(u32int)){
		res = armrdmemwd(fs->jmed, addr+i, &data, 4);
		if(res < 0){
			fprint(2, "Error reading %#8.8ux [%#8.8ux]\n",
				addr+i, i);
			werrstr("read error");
			return -1;
			break;
		}
		nb += res;

		*(u32int *)(rpc->data+i) = data;
		dprint(Dfs, "%d[%#8.8ux] = %#8.8ux \n",
				i, addr+i, data);
	}
	
	dprint(Dfs, "readmem: end of aligned\n");
	/* The end is not aligned */
	if((count-prenb) & 0x3U){
		postnb = (count-prenb)%sizeof(u32int);
		st = addr + 1 + ((count-prenb)& 0x3U);
		res = readbytes(fs->jmed, st, postnb, nb, rpc);
		if(res < 0){
			werrstr("read error");
			return -1;
		}
		nb += res;
		dprint(Dfs, "readmem: end of non aligned\n");
	}
	return nb;
}

static int
writebytes(JMedium *jmed, u32int startaddr, u32int nbytes, u32int dataoff, Fcall *rpc)
{
	u32int i, data;
	int nb, res;

	nb = 0;
	for(i = startaddr; i < startaddr+nbytes; i++){
		data = (char)rpc->data[dataoff];
		res = armwrmemwd(jmed, i, data, 1);
		if(res < 0){
			fprint(2, "Error writing [%#8.8ux]\n",
				i);
			werrstr("read error");
			return -1;
			break;
		}
		nb += res;
		dprint(Dfs, "[B%#8.8ux] = %#2.2ux \n",
				i, data);
	}
	return nb;

}

static int
writemem(Fs *fs, Fcall *rpc)
{
	u32int count, i, addr, *p, prenb, nb, postnb, st;
	int res;

	count = rpc->count;
	addr = rpc->offset;
	
	prenb = 0;
	nb = 0;
	/* not aligned offset */
	if(addr & 0x3U){
		prenb = sizeof(u32int) - (addr & 0x3U);
		res = writebytes(fs->jmed, addr, prenb, 0, rpc);
		if(res < 0){
			werrstr("write error");
			return -1;
		}
		nb += res;
		dprint(Dfs, "writemem: aligned now\n");
	}
	for(i = prenb; i/4 < (count-prenb)/sizeof(u32int); i += sizeof(u32int)){
		p = (u32int *)(rpc->data + i);
		dprint(Dfs, "%d[%#8.8ux] = %#8.8ux \n",
				i, addr+i, *p);
		res = armwrmemwd(fs->jmed, addr + i, *p, 4);
		if(res < 0){
			fprint(2, "Error writing %#8.8ux [%#8.8ux]\n",
				addr+i, i);
			werrstr("write error");
			return -1;
			break;
		}
		nb += res;
	}
	dprint(Dfs, "writemem: end of aligned\n");
	/* not aligned end */
	if((count-prenb)  & 0x3U){
		postnb = (count-prenb)%sizeof(u32int);
		st = addr + 1 + ((count-prenb)& 0x3U);
		res = writebytes(fs->jmed, st, postnb, nb, rpc);
		if(res < 0){
			werrstr("write error");
			return -1;
		}
		nb += res;
		dprint(Dfs, "writemem: end of non aligned\n");
	}
	return nb;
}

/* host to Arm (le) */
static void
setkernur(Ureg *kur, ArmCtxt *context)
{
	int i;
	u32int *p;

	kur->type = context->cpsr&PsrMask;
	hleputl(&kur->type, kur->type);

	kur->psr = context->spsr;
	hleputl(&kur->psr, kur->psr);

	p = (u32int *)kur;
	for(i = 0; i < 14; i++)
		hleputl(p + i, context->r[i]);
	kur->pc = context->r[15];
	hleputl(&kur->pc, kur->pc);
}

typedef struct Regs Regs;
struct Regs {
	Ureg kur;
	MMURegs mmust;
};

static Regs procregs;	/* kludge: just for reading */

/* host to Arm (le) */
static void
setmmust(MMURegs *mmust, ArmCtxt *context)
{
	int i;
	u32int *o, *d;
	d = (u32int *)mmust;
	o = (u32int *) &context->MMURegs;
	for(i = 0; i < sizeof(MMURegs)/sizeof(u32int); i++)
		hleputl(d + i, o[i]);
}

static char*
fsread(Fs *fs, Fcall *rpc)
{
	Fid *f;
	int off, count, len, i;
	uchar *rptr;
	char buf[512];

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(!f->open){
		putfid(fs, f);
		return Enotopen;
	}
	count = rpc->count;
	off = rpc->offset;
	if(f->qid.path == Qroot){
		if(off > 0)
			rpc->count = 0;
		else {
			rpc->count = 0;
			for(i = 0; entries[i].name != nil; i++)
				rpc->count += dostat(entries[i].path,
					rpc->count+(uchar*)rpc->data, count-rpc->count);
		}
		
		putfid(fs, f);
		if(off == 0 && rpc->count <= BIT16SZ)
			return "directory read count too small";
		return nil;
	}
	len = 0;
	if(f->qid.path == Qctl){
		dprint(Dfs, "ctlread\n");
		len = readctl(fs, rpc);
		dprint(Dfs, "ctlread read %d %s\n", len, ctlread);
		if(len < 0){
			putfid(fs, f);
			return "ctl read";
		}
	}
	else if(f->qid.path == Qmem){
		dprint(Dfs, "readmem\n");
		len = readmem(fs, rpc);
		dprint(Dfs, "readmem read %d %s\n", len, ctlread);
		if(len < 0){
			putfid(fs, f);
			return "readmem read";
		}
	}
	else if(f->qid.path == Qkregs){
		dprint(Dfs, "kregs read n %d off %d\n", rpc->count, rpc->offset);
		memset(&procregs, 0, sizeof(Regs));
		setkernur(&procregs.kur, &ctxt);
		setmmust(&procregs.mmust, &ctxt);
		rptr = (uchar*)&procregs;
		len = readbuf(rpc, rptr, sizeof(Regs));
	}
	else if(f->qid.path == Qtext){
		dprint(Dfs, "text\n");
		len = pread(textfd, rpc->data, rpc->count, rpc->offset);
		if(len < 0) {
			rerrstr(buf, sizeof buf);
			fprint(2, "error reading text: %r");
			putfid(fs, f);
			return "text read";
		}
	}
	else if(f->qid.path == Qstatus){
		dprint(Dfs, "status\n");
		len = snprint(buf, sizeof buf, "%-28s%-28s%-28s", "remote", "system", "New");
		for(i = 0; i < 9; i++)
			len += snprint(buf + len, sizeof buf - len, "%-12d", 0);
		len = readbuf(rpc, buf, len);

	}
	putfid(fs, f);
	rpc->count = len;
	return nil;
}

typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
struct Cmdbuf
{
	char	*buf;
	char	**f;
	int	nf;
};

struct Cmdtab
{
	int	index;	/* used by client to switch on result */
	char	*cmd;	/* command name */
	int	narg;	/* expected #args; 0 ==> variadic */
};
/*
 * Generous estimate of number of fields, including terminal nil pointer
 */
static int
ncmdfield(char *p, int n)
{
	int white, nwhite;
	char *ep;
	int nf;

	if(p == nil)
		return 1;

	nf = 0;
	ep = p+n;
	white = 1;	/* first text will start field */
	while(p < ep){
		nwhite = (strchr(" \t\r\n", *p++ & 0xFF) != 0);	/* UTF is irrelevant */
		if(white && !nwhite)						/* beginning of field */
			nf++;
		white = nwhite;
	}
	return nf+1;								/* +1 for nil */
}

/*
 *  parse a command written to a device
 */
static Cmdbuf*
parsecmd(char *p, int n)
{
	Cmdbuf *cb;
	int nf;
	char *sp;

	nf = ncmdfield(p, n);

	/* allocate Cmdbuf plus string pointers plus copy of string including \0 */
	sp = malloc(sizeof(*cb) + nf * sizeof(char*) + n + 1);
	if(sp == nil)
		sysfatal("memory");
	cb = (Cmdbuf*)sp;
	cb->f = (char**)(&cb[1]);
	cb->buf = (char*)(&cb->f[nf]);

	memmove(cb->buf, p, n);

	/* dump new line and null terminate */
	if(n > 0 && cb->buf[n-1] == '\n')
		n--;
	cb->buf[n] = '\0';

	cb->nf = tokenize(cb->buf, cb->f, nf-1);
	cb->f[cb->nf] = nil;

	return cb;
}

/*
 * Look up entry in table
 */

static Cmdtab*
lookupcmd(Cmdbuf *cb, Cmdtab *ctab, int nctab)
{
	int i;
	Cmdtab *ct;

	if(cb->nf == 0){
		werrstr("empty control message");
		return nil;
	}

	for(ct = ctab, i=0; i<nctab; i++, ct++){
		if(strcmp(ct->cmd, "*") !=0)	/* wildcard always matches */
		if(strcmp(ct->cmd, cb->f[0]) != 0)
			continue;
		if(ct->narg != 0 && ct->narg != cb->nf){
			werrstr("bad # args to command");
			return nil;
		}
		return ct;
	}

	werrstr("unknown control message");
	return nil;
}

enum {
	CMcpuid,
	CMdebug,
	CMreset,
	CMstop,
	CMstartstop,
	CMwaitstop,
	CMstart,
	CMdump,
	CMveccatch,
	CMbreakpoint,
};

static Cmdtab ctab[] = {
	CMdebug,	"debug",	2,
	CMreset,	"reset",	1,
	CMcpuid,	"cpuid",	1,
	CMstop,	"stop",	1,
	CMstart,	"start",	1,
	CMdump,	"dump",	3,
	CMveccatch,	"veccatch",	2,
	CMwaitstop,	"waitstop",	1,
	CMstartstop,	"startstop",	1,
	CMbreakpoint,	"breakpoint",	0,	/* 2 or 3 args, optional mask */
};

static int
cmdcpuid(JMedium *jmed)
{
	u32int cpuid;

	cpuid = armidentify(jmed);
	dprint(Dfs, "---- Cpuid --- %8.8ux\n", cpuid);
	if(cpuid == ~0){
		werrstr("not feroceon or nstrs bug...");
		return -1;
	}

	dprint(Dfs, "---- Bypass probe --- \n");
	if(armbpasstest(jmed) < 0){
		fprint(2, "error in bypass\n");
		werrstr("bypass test");
		return -1;
	}
	ctxt.cpuid = cpuid;
	return 0;
}

static int
checkcpuid(JMedium *jmed)
{
	if(ctxt.cpuid != 0)
		return 0;

	return cmdcpuid(jmed);
}


typedef struct VecMode VecMode;
struct VecMode{
	char c;
	u32int mode;
};

static VecMode vmode[] = {
	{'R',	ResetCat},
	{'S',	SWICat},
	{'P',	PAbortCat},
	{'D',	DAbortCat},
	{'I',	IrqCat},
	{'F',	FiqCat},
};

static u32int
vecval(char *conds)
{
	int i, j;
	u32int vcregval;

	vcregval = 0;
	for(i = 0; i < strlen(conds); i++){
		for(j = 0; j < nelem(vmode); j++){
			if(vmode[j].c == conds[i])
				vcregval |= vmode[j].mode;
		}
	}
	
	return vcregval;
}
/* if running, stop, set catch, start, else, set catch, not start*/
static int
cmdveccatch(JMedium *jmed, char *conds)
{
	u32int vcregval;
	int res, wasdebug;

	wasdebug = ctxt.debug;
	if(!wasdebug){
		res = iceenterdebug(jmed, &ctxt);
		if(res< 0){
			werrstr("entering debug to set veccat");
			return -1;
		}
	}

	vcregval = vecval(conds);
	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;
	fprint(Dice, "veccatch: %#8.8ux\n", vcregval);
	res = icesetreg(jmed, VecCatReg, vcregval);
	if(res < 0)
		return -1;

	ctxt.exitreas = VeccatReqReas;
	if(!wasdebug){
		res = iceexitdebug(jmed, &ctxt);
		if(res < 0){
			werrstr("exiting debug to set veccat");
			return -1;
		}
	}

	return 0;
}

/* if running, stop, set breakpoint, start, else, set breakpoint, not start*/
static int
cmdbreakpoint(JMedium *jmed, u32int addr, u32int mask)
{
	int res, wasdebug;

	wasdebug = ctxt.debug;
	if(!wasdebug){	
		if(ctxt.debugreas != BreakReas && ctxt.debugreas != NoReas){
			werrstr("already waiting debug");
			return -1;
		}
		res = iceenterdebug(jmed, &ctxt);
		if(res< 0){
			werrstr("entering debug to set breakpoint");
			return -1;
		}
	}

	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;
	res = icesetreg(jmed, Wp0|AddrValReg, addr);
	if(res < 0)
		return -1;

	res = icesetreg(jmed, Wp0|AddrMskReg, mask);
	if(res < 0)
		return -1;

	res = icesetreg(jmed, Wp0|DataMskReg, 0xffffffff);
	if(res < 0)
		return -1;

	res = icesetreg(jmed, Wp0|CtlMskReg, 0xff&~DataWPCtl);
	if(res < 0)
		return -1;
	res = icesetreg(jmed, Wp0|CtlValReg, EnableWPCtl);
	if(res < 0)
		return -1;
	fprint(Dice, "breakpoint: addr %#8.8ux msk %#8.8ux \n", addr, mask);

	ctxt.exitreas = BreakReqReas;
	if(!wasdebug){
		res = iceexitdebug(jmed, &ctxt);
		if(res < 0){
			werrstr("exiting debug to set breakpoint");
			return -1;
		}
	}
	return 0;
}

static char dbgstr[4*1024];

static int
cmdstop(JMedium *jmed)
{
	int res;

	res = iceenterdebug(jmed, &ctxt);
	if(res < 0){
		return -1;
	}

	ctxt.debug = icedebugstate(fs.jmed);
	return 0;
}

static int
cmdwaitstop(JMedium *jmed)
{
	int res;

	res = icewaitentry(jmed, &ctxt);
	if(res < 0){
		werrstr("timeout waiting for entry to debug state");
		return -1;
	}
	return 0;
}

static int
cmdstart(JMedium *jmed)
{
	int res;
	res = iceexitdebug(jmed, &ctxt);
	if(res < 0){
		werrstr("could not exit debug");
		return -1;
	}
	ctxt.debug = icedebugstate(fs.jmed);
	return 0;
}

static int
cmddump(JMedium *jmed, u32int start, u32int end)
{
	int res, i;
	u32int data;
	debug[Dctxt] = 1;


	for(i = 0; i < end-start; i+=4){
		res = armrdmemwd(jmed, start+i, &data, 4);
		if(res < 0){
			fprint(2, "Error reading %#8.8ux [%#8.8ux]\n", start+i, i);
			werrstr("read error");
			return -1;
			break;
		}
		dprint(Dfs, "%d[%#8.8ux] = %#8.8ux \n",
				i, start+i, data);
	}
	return 0;
}

static int
cmdhwreset(Fs *fs)
{
	JMedium *jmed;

	jmed = fs->jmed;
	if(jmed->flush(jmed->mdata)){
		werrstr("flush");
		return -1;
	}

	fs->jmed = resetmpsse(fs->jmed);
	if(fs->jmed == nil)
		return -1;
	ctxt.debug = 0;

	jmed = fs->jmed;
	/* BUG make this medium independant..., add a send 5 TMS to interface */
	if(pushcmd(jmed->mdata, "TmsCsOut EdgeDown LSB B0x7 0x7f") < 0) {
		werrstr("going to reset");
		return -1;
	}
	if(jmed->flush(jmed->mdata)){
		werrstr("flush");
		return -1;
	}
	sleep(1000);
	jmed->resets(jmed->mdata, 1, 0);
	sleep(200);
	jmed->resets(jmed->mdata, 1, 1);
	sleep(200);
	jmed->resets(jmed->mdata, 0, 1);
	sleep(200);
	jmed->resets(jmed->mdata, 0, 0);
	sleep(200);
	if(jmed->flush(jmed->mdata)){
		werrstr("flush");
		return -1;
	}
	memset(&ctxt, 0, sizeof(ArmCtxt));
	return 0;
}

static int
cmdstartstop(JMedium *jmed)
{
	if(cmdstart(jmed) < 0){
		werrstr("starting");
		return -1;
	}
	if(cmdstop(jmed) < 0){
		werrstr("stopping");
		return -1;
	}
	return 0;
}


static int
runcmd(Fs *fs, char *data, int count)
{
	Cmdtab *t;
	Cmdbuf *cb;
	int res;
	u32int st, end, addr, msk;

	cb = parsecmd(data, count);
	if(cb->nf < 1){
		werrstr("empty control message");
		free(cb);
		return -1;
	}

	t = lookupcmd(cb, ctab, nelem(ctab));
	if(t == nil){
		free(cb);
		return -1;
	}

	if(t->index != CMreset && t->index != CMcpuid &&
		t->index != CMdebug && 
		checkcpuid(fs->jmed) < 0)
		return -1;

	switch(t->index){
	case CMdebug:
		res = cmdsetdebug((uchar *)cb->f[1]);
		break;
	case CMreset:
		dprint(Dfs, "reset\n");
		res = cmdhwreset(fs);
		break;
	case CMcpuid:
		dprint(Dfs, "cpuid\n");
		res = checkcpuid(fs->jmed);
		break;
	case CMstop:
		dprint(Dfs, "stop\n");
		res = cmdstop(fs->jmed);
		break;
	case CMwaitstop:
		dprint(Dfs, "waitstop\n");
		res = cmdwaitstop(fs->jmed);
		break;
	case CMstart:
		dprint(Dfs, "start\n");
		res = cmdstart(fs->jmed);
		break;
	case CMdump:
		st = atol(cb->f[1]);
		end = atol(cb->f[2]);
		dprint(Dfs, "dump %#8.8ux %#8.8ux\n", st, end);
		res = cmddump(fs->jmed, st, end);
		break;
	case CMveccatch:
		dprint(Dfs, "veccatch %s\n", cb->f[1]);
		res = cmdveccatch(fs->jmed, cb->f[1]);
		break;
	case CMbreakpoint:
		addr = atol(cb->f[1]);
		if(cb->nf > 0 && cb->nf == 3)
			msk = atol(cb->f[2]);
		else	if(cb->nf > 0 && cb->nf == 2)
				msk = 0;
		else {
			res = -1;
			goto Exit;
		}
	
		dprint(Dfs, "breakpoint addr %#8.8ux mask %#8.8ux\n", addr, msk);
		res = cmdbreakpoint(fs->jmed, addr, msk);
		break;
	default:
		res = -1;
	}
Exit:
	free(cb);
	return res;
}

/* Arm (le) order to host */
static void
getkernur(ArmCtxt *ct, Ureg *kur)
{
	int i;

	ct->cpsr = (ct->cpsr&~PsrMask) | kur->type;
	ct->cpsr = lehgetl(&ct->cpsr);

	ct->spsr = kur->psr;
	ct->spsr = lehgetl(&ct->spsr);

	memmove(ct->r, kur, 14*sizeof(u32int));
	ct->r[15] = kur->pc;
	for(i = 0; i < 15; i++)
		ct->r[i] = lehgetl(ct->r + i);
}

static int
setkregs(Fs *fs, Fcall *rpc)
{
	Ureg kur;
	ArmCtxt lc;
	u32int mask, *lp, *gp;
	int res, nb, i;
	char *p;
	JMedium *jmed;

	jmed = fs->jmed;

	nb = 0;
	lc = ctxt;
	setkernur(&kur, &ctxt);
	p = (char *)&kur;
	if(rpc->count + rpc->offset > sizeof(Ureg))	/* cannot write mmuregs*/
		return -1;
	else
		memmove(p + rpc->offset, rpc->data, rpc->count);

	getkernur(&ctxt, &kur);

	/* BUG? hmm, order matters here, if I get both I am not sure of what to do */
	if(ctxt.cpsr != lc.cpsr){
		res = armsetexec(jmed, 1, &ctxt.cpsr, ARMLDMIA|0x0001);
		if(res < 0)
			return -1;
		res = armgofetch(jmed, ARMMSRr0CPSR, 0);
		if(res < 0)
			return -1;
		nb += sizeof(u32int);
	}
		
	if(ctxt.spsr != lc.spsr){
		res = armsetexec(jmed, 1, &ctxt.spsr, ARMLDMIA|0x0001);
		if(res < 0)
			return -1;
		res = armgofetch(jmed, ARMMSRr0SPSR, 0);
		if(res < 0)
			return -1;
		nb += sizeof(u32int);
	}
		


	/* last I update the registers */
	lp = (u32int *)&lc;
	gp = (u32int *)&ctxt;
	mask = 0;
	for(i = 0; i < 16; i++){
		if(lp[i] != gp[i]){	/* see which ones changed */
			mask |= (1 << i);
			nb += sizeof(u32int);
		}
	}
	mask |= 0x0001;	/* this one I contaminated myself */
	res = armsetregs(jmed, mask, ctxt.r);
	if(res < 0)
		return -1;
	return nb;
}

static char*
fswrite(Fs *fs, Fcall *rpc)
{
	Fid *f;
	int res, len;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(!f->open){
		putfid(fs, f);
		return Enotopen;
	}

	if(f->qid.path == Qctl){
		res = runcmd(fs, rpc->data, rpc->count);
		if(res < 0){
			fprint(2, "error %r\n");
			putfid(fs, f);
			return Ebadcmd;
		}
	}
	else if(f->qid.path == Qmem){
		len = writemem(fs, rpc);
		if(len < 0){
			putfid(fs, f);
			return "reading memory";
		}
	}
	else if(f->qid.path == Qkregs){
		dprint(Dfs, "kregs write n %d off %d\n", rpc->count, rpc->offset);
		len = setkregs(fs, rpc);
		if(len < 0){
			putfid(fs, f);
			return "writing kregs";
		}
	}
	else {
		putfid(fs, f);
		return Eperm;
	}
	putfid(fs, f);
	return nil;
}

static char *
fsclunk(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = getfid(fs, rpc->fid);
	if(f != nil){
		f->attached = 0;
		putfid(fs, f);
	}
	return nil;
}

static char *
fsremove(Fs *, Fcall *)
{
	return Eperm;
}

static char *
fsstat(Fs *fs, Fcall *rpc)
{
	Fid *f;

	f = getfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	rpc->stat = fs->statbuf;
	rpc->nstat = dostat(f->qid.path, rpc->stat, sizeof fs->statbuf);
	putfid(fs, f);
	if(rpc->nstat <= BIT16SZ)
		return "stat count too small";
	return nil;
}

static char *
fswstat(Fs *, Fcall *)
{
	return Eperm;
}

static int
dostat(int path, uchar *buf, int nbuf)
{
	Dir d;

	switch(path){
	case Qroot:
		d.name = ".";
		d.mode = DMDIR|0555;
		d.qid.type = QTDIR;
		break;
	case Qctl:
		d.name = "ctl";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qmem:
		d.name = "mem";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qkregs:
		d.name = "kregs";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qfpregs:
		d.name = "fpregs";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qproc:
		d.name = "proc";
		d.mode = 0444;
		d.qid.type = QTFILE;
		break;
	case Qregs:
		d.name = "regs";
		d.mode = 0666;
		d.qid.type = QTFILE;
		break;
	case Qtext:
		d.name = "text";
		d.mode = 0444;
		d.qid.type = QTFILE;
		break;
	case Qstatus:
		d.name = "status";
		d.mode = 0444;
		d.qid.type = QTFILE;
		break;
	}
	d.qid.path = path;
	d.qid.vers = 0;
	d.length = 0;
	d.uid = d.gid = d.muid = "none";
	d.atime = d.mtime = time(nil);
	return convD2M(&d, buf, nbuf);
}

static void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[1024];

	write(2, "jtagfs: ", 8);
	va_start(arg, fmt);
	vseprint(buf, buf+1024, fmt, arg);
	va_end(arg);
	write(2, buf, strlen(buf));
	write(2, "\n", 1);
	exits(fmt);
}

static void *
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal("out of memory");
	memset(p, 0, n);
	return p;
}

static void
usage(void)
{
	fprint(2, "usage: jtagfs  [-t text] [-d debug] [-m mountpoint] [-s srvfile] jtag\n");
	exits("usage");
}
