#include "all.h"

static char* cname[CMAX] = 
{
	[CEQ] "==",
	[CGE] ">=",
	[CGT] "> ",
	[CLT] "< ",
	[CLE] "<=",
	[CNE] "!=",
};

vlong calltime[Tmax];
ulong ncalls[Tmax];

char* callname[] =
{
	/* ix requests */
	[IXTversion]	"Tversion",
	[IXRversion]	"Rversion",
	[IXTattach]	"Tattach",
	[IXRattach]	"Rattach",
	[IXTfid]	"Tfid",
	[IXRfid]	"Rfid",
	[__IXunused__]	"__IXunused__",
	[IXRerror]	"Rerror",
	[IXTclone]	"Tclone",
	[IXRclone]	"Rclone",
	[IXTwalk]	"Twalk",
	[IXRwalk]	"Rwalk",
	[IXTopen]	"Topen",
	[IXRopen]	"Ropen",
	[IXTcreate]	"Tcreate",
	[IXRcreate]	"Rcreate",
	[IXTread]	"Tread",
	[IXRread]	"Rread",
	[IXTwrite]	"Twrite",
	[IXRwrite]	"Rwrite",
	[IXTclunk]	"Tclunk",
	[IXRclunk]	"Rclunk",
	[IXTremove]	"Tremove",
	[IXRremove]	"Rremove",
	[IXTattr]	"Tattr",
	[IXRattr]	"Rattr",
	[IXTwattr]	"Twattr",
	[IXRwattr]	"Rwattr",
	[IXTcond]	"Tcond",
	[IXRcond]	"Rcond",
	[IXTmove]	"Tmove",
	[IXRmove]	"Rmove",

	/* 9p requests */
	[Tversion]	"Tversion",
	[Rversion]	"Rversion",
	[Tauth]		"Tauth",
	[Rauth]		"Rauth",
	[Tattach]	"Tattach",
	[Rattach]	"Rattach",
	[Terror]	"Terror",
	[Rerror]	"Rerror",
	[Tflush]	"Tflush",
	[Rflush]	"Rflush",
	[Twalk]		"Twalk",
	[Rwalk]		"Rwalk",
	[Topen]		"Topen",
	[Ropen]		"Ropen",
	[Tcreate]	"Tcreate",
	[Rcreate]	"Rcreate",
	[Tread]		"Tread",
	[Rread]		"Rread",
	[Twrite]		"Twrite",
	[Rwrite]		"Rwrite",
	[Tclunk]	"Tclunk",
	[Rclunk]	"Rclunk",
	[Tremove]	"Tremove",
	[Rremove]	"Rremove",
	[Tstat]		"Tstat",
	[Rstat]		"Rstat",
	[Twstat]	"Twstat",
	[Rwstat]	"Rwstat",
};

static uchar*
pstring(uchar *p, char *s)
{
	uint n;

	if(s == nil){
		PBIT16(p, 0);
		p += BIT16SZ;
		return p;
	}

	n = strlen(s);
	/*
	 * We are moving the string before the length,
	 * so you can S2M a struct into an existing message
	 */
	memmove(p + BIT16SZ, s, n);
	PBIT16(p, n);
	p += n + BIT16SZ;
	return p;
}

static uint
stringsz(char *s)
{
	if(s == nil)
		return BIT16SZ;

	return BIT16SZ+strlen(s);
}

/*
 * Does NOT count the data bytes added past the packed
 * message for IXRread, IXTwrite. This is so to save copying.
 * The caller is expected to copy the final data in-place and
 * adjust the total message length.
 */
uint
ixpackedsize(IXcall *f)
{
	uint n;

	n = BIT8SZ;	/* type */

	switch(f->type){
	case IXTversion:
	case IXRversion:
		n += BIT32SZ;
		n += stringsz(f->version);
		break;

	case IXTattach:
		n += stringsz(f->uname);
		n += stringsz(f->aname);
		break;
	case IXRattach:
		n += BIT32SZ;
		break;

	case IXTfid:
		n += BIT32SZ;
		break;
	case IXRfid:
		break;

	case IXRerror:
		n += stringsz(f->ename);
		break;

	case IXTclone:
		n += BIT8SZ;
		break;
	case IXRclone:
		n += BIT32SZ;
		break;

	case IXTwalk:
		n += stringsz(f->wname);
		break;
	case IXRwalk:
		break;

	case IXTopen:
		n += BIT8SZ;
		break;
	case IXRopen:
		break;

	case IXTcreate:
		n += stringsz(f->name);
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case IXRcreate:
		break;

	case IXTread:
		n += BIT16SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		break;
	case IXRread:
		/* data follows; not counted */
		break;

	case IXTwrite:
		n += BIT64SZ;
		n += BIT64SZ;
		/* data follows; not counted */
		break;
	case IXRwrite:
		n += BIT64SZ;
		n += BIT32SZ;
		break;

	case IXTclunk:
	case IXRclunk:
	case IXTremove:
	case IXRremove:
		break;

	case IXTattr:
		n += stringsz(f->attr);
		break;
	case IXRattr:
		n += f->nvalue;
		break;

	case IXTwattr:
		n += stringsz(f->attr);
		n += f->nvalue;
		break;
	case IXRwattr:
		break;

	case IXTcond:
		n += BIT8SZ;
		n += stringsz(f->attr);
		n += f->nvalue;
		break;
	case IXRcond:
		break;

	case IXTmove:
		n += BIT32SZ;
		n += stringsz(f->newname);
		break;
	case IXRmove:
		break;

	default:
		sysfatal("packedsize: unknown type %d", f->type);

	}
	return n;
}

uint
ixpack(IXcall *f, uchar *ap, uint nap)
{
	uchar *p;
	uint size;

	size = ixpackedsize(f);
	if(size == 0 || size > nap)
		return 0;

	p = (uchar*)ap;

	PBIT8(p, f->type);
	p += BIT8SZ;

	switch(f->type){
	case IXTversion:
	case IXRversion:
		PBIT32(p, f->msize);
		p += BIT32SZ;
		p  = pstring(p, f->version);
		break;

	case IXTattach:
		p  = pstring(p, f->uname);
		p  = pstring(p, f->aname);
		break;
	case IXRattach:
		PBIT32(p, f->fid);
		p += BIT32SZ;
		break;

	case IXTfid:
		PBIT32(p, f->fid);
		p += BIT32SZ;
		break;
	case IXRfid:
		break;

	case IXRerror:
		p  = pstring(p, f->ename);
		break;

	case IXTclone:
		PBIT8(p, f->cflags);
		p += BIT8SZ;
		break;
	case IXRclone:
		PBIT32(p, f->fid);
		p += BIT32SZ;
		break;

	case IXTwalk:
		p  = pstring(p, f->wname);
		break;
	case IXRwalk:
		break;

	case IXTopen:
		PBIT8(p, f->mode);
		p += BIT8SZ;
		break;
	case IXRopen:
		break;

	case IXTcreate:
		p  = pstring(p, f->name);
		PBIT32(p, f->perm);
		p += BIT32SZ;
		PBIT8(p, f->mode);
		p += BIT8SZ;
		break;
	case IXRcreate:
		break;

	case IXTread:
		PBIT16(p, f->nmsg);
		p += BIT16SZ;
		PBIT64(p, f->offset);
		p += BIT64SZ;
		PBIT32(p, f->count);
		p += BIT32SZ;
		break;
	case IXRread:
		/* data follows; not packed */
		break;

	case IXTwrite:
		PBIT64(p, f->offset);
		p += BIT64SZ;
		PBIT64(p, f->endoffset);
		p += BIT64SZ;
		/* data follows; not packed */
		break;
	case IXRwrite:
		PBIT64(p, f->offset);
		p += BIT64SZ;
		PBIT32(p, f->count);
		p += BIT32SZ;
		break;

	case IXTclunk:
	case IXRclunk:
	case IXTremove:
	case IXRremove:
		break;

	case IXTattr:
		p  = pstring(p, f->attr);
		break;
	case IXRattr:
		memmove(p, f->value, f->nvalue);
		p += f->nvalue;
		break;

	case IXTwattr:
		p  = pstring(p, f->attr);
		memmove(p, f->value, f->nvalue);
		p += f->nvalue;
		break;
	case IXRwattr:
		break;

	case IXTcond:
		if(f->op >= CMAX){
			werrstr("unknown cond op");
			return 0;
		}
		PBIT8(p, f->op);
		p += BIT8SZ;
		p  = pstring(p, f->attr);
		memmove(p, f->value, f->nvalue);
		p += f->nvalue;
		break;
	case IXRcond:
		break;

	case IXTmove:
		PBIT32(p, f->dirfid);
		p += BIT32SZ;
		p  = pstring(p, f->newname);
		break;
	case IXRmove:
		break;

	default:
		sysfatal("pack: type %d", f->type);

	}
	if(size != p-ap)
		return 0;
	return size;
}

static uchar*
gstring(uchar *p, uchar *ep, char **s)
{
	uint n;

	if(p+BIT16SZ > ep)
		return nil;
	n = GBIT16(p);
	p += BIT16SZ - 1;
	if(p+n+1 > ep)
		return nil;
	/* move it down, on top of count, to make room for '\0' */
	memmove(p, p + 1, n);
	p[n] = '\0';
	*s = (char*)p;
	p += n+1;
	return p;
}

uint
ixunpack(uchar *ap, uint nap, IXcall *f)
{
	uchar *p, *ep;

	p = ap;
	ep = p + nap;

	if(p+BIT8SZ > ep){
		werrstr("msg too short");
		return 0;
	}

	f->type = GBIT8(p);
	p += BIT8SZ;

	switch(f->type){
	case IXTversion:
	case IXRversion:
		if(p+BIT32SZ > ep)
			return 0;
		f->msize = GBIT32(p);
		p += BIT32SZ;
		p = gstring(p, ep, &f->version);
		break;

	case IXTattach:
		p = gstring(p, ep, &f->uname);
		if(p == nil)
			return 0;
		p = gstring(p, ep, &f->aname);
		break;
	case IXRattach:
		if(p+BIT32SZ > ep)
			return 0;
		f->fid = GBIT32(p);
		p += BIT32SZ;
		break;

	case IXTfid:
		if(p+BIT32SZ > ep)
			return 0;
		f->fid = GBIT32(p);
		p += BIT32SZ;
		break;
	case IXRfid:
		break;

	case IXRerror:
		p = gstring(p, ep, &f->ename);
		break;

	case IXTclone:
		if(p+BIT8SZ > ep)
			return 0;
		f->cflags = GBIT8(p);
		p += BIT8SZ;
		break;
	case IXRclone:
		if(p+BIT32SZ > ep)
			return 0;
		f->fid = GBIT32(p);
		p += BIT32SZ;
		break;

	case IXTwalk:
		p  = gstring(p, ep, &f->wname);
		break;
	case IXRwalk:
		break;

	case IXTopen:
		if(p+BIT8SZ > ep)
			return 0;
		f->mode = GBIT8(p);
		p += BIT8SZ;
		break;
	case IXRopen:
		break;

	case IXTcreate:
		p = gstring(p, ep, &f->name);
		if(p == nil)
			break;
		if(p+BIT32SZ+BIT8SZ > ep)
			return 0;
		f->perm = GBIT32(p);
		p += BIT32SZ;
		f->mode = GBIT8(p);
		p += BIT8SZ;
		break;
	case IXRcreate:
		break;

	case IXTread:
		if(p+BIT16SZ+BIT64SZ+BIT32SZ > ep)
			return 0;
		f->nmsg = GBIT16(p);
		p += BIT16SZ;
		f->offset = GBIT64(p);
		p += BIT64SZ;
		f->count = GBIT32(p);
		p += BIT32SZ;
		break;
	case IXRread:
		f->data = p;
		f->count = ep - p;
		break;

	case IXTwrite:
		if(p+BIT64SZ > ep)
			return 0;
		f->offset = GBIT64(p);
		p += BIT64SZ;
		f->endoffset = GBIT64(p);
		p += BIT64SZ;
		f->data = p;
		f->count = ep - p;
		break;
	case IXRwrite:
		if(p+BIT32SZ+BIT64SZ > ep)
			return 0;
		f->offset = GBIT64(p);
		p += BIT64SZ;
		f->count = GBIT32(p);
		p += BIT32SZ;
		break;

	case IXTclunk:
	case IXRclunk:
	case IXTremove:
	case IXRremove:
		break;

	case IXTattr:
		p = gstring(p, ep, &f->attr);
		break;
	case IXRattr:
		f->value = p;
		f->nvalue = ep - p;
		break;

	case IXTwattr:
		p = gstring(p, ep, &f->attr);
		if(p == nil)
			return 0;
		f->value = p;
		f->nvalue = ep - p;
		break;
	case IXRwattr:
		break;

	case IXTcond:
		if(p+BIT8SZ > ep)
			return 0;
		f->op = GBIT8(p);
		if(f->op >= CMAX){
			werrstr("unknown cond op");
			return 0;
		}
		p += BIT8SZ;
		p = gstring(p, ep, &f->attr);
		if(p == nil)
			return 0;
		f->value = p;
		f->nvalue = ep - p;
		break;
	case IXRcond:
		break;

	case IXTmove:
		if(p+BIT32SZ > ep)
			return 0;
		f->dirfid = GBIT32(p);
		p += BIT32SZ;
		p = gstring(p, ep, &f->newname);
		break;
	case IXRmove:
		break;

	default:
		werrstr("unpack: unknown type %d", f->type);
		return 0;
	}

	if(p==nil || p>ep || p == ap){
		werrstr("unpack: p %#p ep %#p", p, ep);
		return 0;
	}
	return p - ap;
}

int
rpcfmt(Fmt *fmt)
{
	Rpc *rpc;

	rpc = va_arg(fmt->args, Rpc*);
	if(rpc == nil)
		return fmtprint(fmt, "<nil>");
	if(rpc->t.type == 0)
		return fmtprint(fmt, "Tnull");
	if(rpc->t.type < nelem(callname) && callname[rpc->t.type])
		return fmtprint(fmt, "%s tag %ud", callname[rpc->t.type], rpc->t.tag);
	return fmtprint(fmt, "type=%d??? tag %ud", rpc->t.type, rpc->t.tag);
}


/*
 * dump out count (or DUMPL, if count is bigger) bytes from
 * buf to ans, as a string if they are all printable,
 * else as a series of hex bytes
 */
#define DUMPL 64

static uint
dumpsome(char *ans, char *e, void *b, long count)
{
	int i, printable;
	char *p;
	char *buf;

	buf = b;
	if(buf == nil){
		seprint(ans, e, "<no data>");
		return strlen(ans);
	}
	printable = 1;
	if(count > DUMPL)
		count = DUMPL;
	for(i=0; i<count && printable; i++)
		if((buf[i]<32 && buf[i] !='\n' && buf[i] !='\t') || (uchar)buf[i]>127)
			printable = 0;
	p = ans;
	*p++ = '\'';
	if(printable){
		if(count > e-p-2)
			count = e-p-2;
		for(; count > 0; count--, p++, buf++)
			if(*buf == '\n' || *buf == '\t')
				*p = ' ';
			else
				*p = *buf;
	}else{
		if(2*count > e-p-2)
			count = (e-p-2)/2;
		for(i=0; i<count; i++){
			if(i>0 && i%4==0)
				*p++ = ' ';
			sprint(p, "%2.2ux", (uchar)buf[i]);
			p += 2;
		}
	}
	*p++ = '\'';
	*p = 0;
	return p - ans;
}

/*
 * Uses a buffer so prints are not mixed with other debug prints.
 */
int
ixcallfmt(Fmt *fmt)
{
	IXcall *f;
	int type;
	char buf[512];
	char *e, *s;

	e = buf+sizeof(buf);
	f = va_arg(fmt->args, IXcall*);
	type = f->type;
	if(type < IXTversion || type >= IXTmax)
		return fmtprint(fmt, "<TYPE %d>", type);
	s = seprint(buf, e, "%s", callname[type]);
	switch(type){
	case IXTversion:
	case IXRversion:
		seprint(s, e, " msize %ud version '%s'", f->msize, f->version);
		break;
		break;

	case IXTattach:
		seprint(s, e, " uname '%s' aname '%s'", f->uname, f->aname);
		break;
	case IXRattach:
		seprint(s, e, " fid %d", f->fid);
		break;

	case IXTfid:
		seprint(s, e, " fid %ud", f->fid);
		break;
	case IXRfid:
		break;

	case IXRerror:
		seprint(s, e, " ename '%s'", f->ename);
		break;

	case IXTclone:
		seprint(s, e, " cflags %#x", f->cflags);
		break;
	case IXRclone:
		seprint(s, e, " fid %d", f->fid);
		break;

	case IXTwalk:
		seprint(s, e, " '%s'", f->wname);
		break;
	case IXRwalk:
		break;

	case IXTopen:
		seprint(s, e, " mode %d", f->mode);
		break;
	case IXRopen:
		break;

	case IXTcreate:
		seprint(s, e, " name '%s' perm %M mode %d",
			f->name, (ulong)f->perm, f->mode);
		break;
	case IXRcreate:
		break;

	case IXTread:
		seprint(s, e, " nmsg %d offset %lld count %ud",
			f->nmsg, f->offset, f->count);
		break;
	case IXRread:
		s = seprint(s, e, " count %ud ", f->count);
		dumpsome(s, e, f->data, f->count);
		break;

	case IXTwrite:
		s = seprint(s, e, " offset %lld endoffset %lld count %ud ",
			f->offset, f->endoffset, f->count);
		dumpsome(s, e, f->data, f->count);
		break;
	case IXRwrite:
		seprint(s, e, " offset %lld count %ud", f->offset, f->count);
		break;

	case IXTclunk:
	case IXRclunk:
	case IXTremove:
	case IXRremove:
		break;

	case IXTattr:
		seprint(s, e, " attr '%s'", f->attr);
		break;
	case IXRattr:
		s = seprint(s, e, " value ");
		dumpsome(s, e, f->value, f->nvalue);
		break;

	case IXTwattr:
		s = seprint(s, e, " attr '%s' value ", f->attr);
		dumpsome(s, e, f->value, f->nvalue);
		break;
	case IXRwattr:
		break;

	case IXTcond:
		s = "??";
		if(f->op < CMAX)
			s = cname[f->op];
		s = seprint(s, e, " op '%s'", s);
		s = seprint(s, e, "attr '%s' value ", f->attr);
		dumpsome(s, e, f->value, f->nvalue);
		break;
	case IXRcond:
		break;

	case IXTmove:
		seprint(s, e, " dirfid %d newname '%s'", f->dirfid, f->newname);
		break;
	case IXRmove:
		break;

	}
	return fmtstrcpy(fmt, buf);
}

