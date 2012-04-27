#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

static char magic[] = { 0xff, 'S', 'M', 'B' };

Session *
cifsdial(char *host, char *called, char *sysname)
{
	int nbt, fd;
	char *addr;
	Session *s;

	if(Debug)
		fprint(2, "cifsdial: host=%s called=%s sysname=%s\n", host, called, sysname);

	if((addr = netmkaddr(host, "tcp", "cifs")) == nil)
		return nil;

	nbt = 0;
	if((fd = dial(addr, nil, nil, nil)) == -1){
		nbt = 1;
		if((fd = nbtdial(host, called, sysname)) == -1)
			return nil;
	}

	s = emalloc9p(sizeof(Session));
	memset(s, 0, sizeof(Session));

	s->fd = fd;
	s->nbt = nbt;
	s->mtu = MTU;
	s->pid = getpid();
	s->mid = time(nil) ^ getpid();
	s->uid = NO_UID;
	s->seq = 0;
	s->seqrun = 0;
	s->secmode = SECMODE_SIGN_ENABLED;	/* hope for the best */
	s->flags2 = FL2_KNOWS_LONG_NAMES | FL2_HAS_LONG_NAMES | FL2_PAGEING_IO;
	s->macidx = -1;

	return s;
}

void
cifsclose(Session *s)
{
	if(s->fd)
		close(s->fd);
	free(s);
}

Pkt *
cifshdr(Session *s, Share *sp, int cmd)
{
	Pkt *p;
	int sign, tid, dfs;

	dfs = 0;
	tid = NO_TID;
	Active = IDLE_TIME;
	werrstr("");
	sign = s->secmode & SECMODE_SIGN_ENABLED? FL2_PACKET_SIGNATURES: 0;

	if(sp){
		tid = sp->tid;
// FIXME!		if(sp->options & SMB_SHARE_IS_IN_DFS)
// FIXME!			dfs = FL2_DFS;
	}

	p = emalloc9p(sizeof(Pkt) + MTU);
	memset(p, 0, sizeof(Pkt) +MTU);

	p->buf = (uchar *)p + sizeof(Pkt);
	p->s = s;

	qlock(&s->seqlock);
	if(s->seqrun){
		p->seq = s->seq;
		s->seq = (s->seq + 2) % 0x10000;
	}
	qunlock(&s->seqlock);

	nbthdr(p);
	pmem(p, magic, nelem(magic));
	p8(p, cmd);
	pl32(p, 0);				/* status (error) */
	p8(p, FL_CASELESS_NAMES | FL_CANNONICAL_NAMES); /* flags */
	pl16(p, s->flags2 | dfs | sign);	/* flags2 */
	pl16(p, (s->pid >> 16) & 0xffff);	/* PID MS bits */
	pl32(p, p->seq);			/* MAC / sequence number */
	pl32(p, 0);				/* MAC */
	pl16(p, 0);				/* padding */

	pl16(p, tid);
	pl16(p, s->pid & 0xffff);
	pl16(p, s->uid);
	pl16(p, s->mid);

	p->wordbase = p8(p, 0);		/* filled in by pbytes() */

	return p;
}

void
pbytes(Pkt *p)
{
	int n;

	assert(p->wordbase != nil);	/* cifshdr not called */
	assert(p->bytebase == nil);	/* called twice */

	n = p->pos - p->wordbase;
	assert(n % 2 != 0);		/* even addr */
	*p->wordbase = n / 2;

	p->bytebase = pl16(p, 0);	/* filled in by cifsrpc() */
}

static void
dmp(int seq, uchar *buf)
{
	int i;

	if(seq == 99)
		print("\n   ");
	else
		print("%+2d ", seq);
	for(i = 0; i < 8; i++)
		print("%02x ", buf[i] & 0xff);
	print("\n");
}

int
cifsrpc(Pkt *p)
{
	int flags2, got, err;
	uint tid, uid, seq;
	uchar *pos;
	char m[nelem(magic)];

	pos = p->pos;
	if(p->bytebase){
		p->pos = p->bytebase;
		pl16(p, pos - (p->bytebase + 2)); /* 2 = sizeof bytecount */
	}
	p->pos = pos;

	if(p->s->secmode & SECMODE_SIGN_ENABLED)
		macsign(p, p->seq);

	qlock(&p->s->rpclock);
	got = nbtrpc(p);
	qunlock(&p->s->rpclock);
	if(got == -1)
		return -1;

	gmem(p, m, nelem(magic));
	if(memcmp(m, magic, nelem(magic)) != 0){
		werrstr("cifsrpc: bad magic number in packet %20ux%02ux%02ux%02ux",
			m[0], m[1], m[2], m[3]);
		return -1;
	}

	g8(p);				/* cmd */
	err = gl32(p);			/* errcode */
	g8(p);				/* flags */
	flags2 = gl16(p);		/* flags2 */
	gl16(p);			/* PID MS bits */
	seq = gl32(p);			/* reserved */
	gl32(p);			/* MAC (if in use) */
	gl16(p);			/* Padding */
	tid = gl16(p);			/* TID */
	gl16(p);			/* PID lsbs */
	uid = gl16(p);			/* UID */
	gl16(p);			/* mid */
	g8(p);				/* word count */

	if(p->s->secmode & SECMODE_SIGN_ENABLED){
		if(macsign(p, p->seq+1) != 0 && p->s->seqrun){
			werrstr("cifsrpc: invalid packet signature");
print("MAC signature bad\n");
// FIXME: for debug only			return -1;
		}
	}else{
		/*
		 * We allow the sequence number of zero as some old samba
		 * servers seem to fall back to this unexpectedly
		 * after reporting sequence numbers correctly for a while.
		 *
		 * Some other samba servers seem to always report a sequence
		 * number of zero if MAC signing is disabled, so we have to
		 * catch that too.
		 */
		if(p->s->seqrun && seq != p->seq && seq != 0){
			print("%ux != %ux bad sequence number\n", seq, p->seq);
			return -1;
		}
	}

	p->tid = tid;
	if(p->s->uid == NO_UID)
		p->s->uid = uid;

	if(flags2 & FL2_NT_ERRCODES){
		/* is it a real error rather than info/warning/chatter? */
		if((err & 0xF0000000) == 0xC0000000){
			werrstr("%s", nterrstr(err));
			return -1;
		}
	}else{
		if(err){
			werrstr("%s", doserrstr(err));
			return -1;
		}
	}
	return got;
}


/*
 * Some older servers (old samba) prefer to talk older
 * dialects but if given no choice they will talk the
 * more modern ones, so we don't give them the choice.
 */
int
CIFSnegotiate(Session *s, long *svrtime, char *domain, int domlen, char *cname,
	int cnamlen)
{
	int d, i;
	char *ispeak = "NT LM 0.12";
	static char *dialects[] = {
//		{ "PC NETWORK PROGRAM 1.0"},
//		{ "MICROSOFT NETWORKS 1.03"},
//		{ "MICROSOFT NETWORKS 3.0"},
//		{ "LANMAN1.0"},
//		{ "LM1.2X002"},
//		{ "NT LANMAN 1.0"},
		{ "NT LM 0.12" },
	};
	Pkt *p;

	p = cifshdr(s, nil, SMB_COM_NEGOTIATE);
	pbytes(p);
	for(i = 0; i < nelem(dialects); i++){
		p8(p, STR_DIALECT);
		pstr(p, dialects[i]);
	}

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	d = gl16(p);
	if(d < 0 || d > nelem(dialects)){
		werrstr("no CIFS dialect in common");
		free(p);
		return -1;
	}

	if(strcmp(dialects[d], ispeak) != 0){
		werrstr("%s dialect unsupported", dialects[d]);
		free(p);
		return -1;
	}

	s->secmode = g8(p);			/* Security mode */

	gl16(p);				/* Max outstanding requests */
	gl16(p);				/* Max VCs */
	s->mtu = gl32(p);			/* Max buffer size */
	gl32(p);				/* Max raw buffer size (depricated) */
	gl32(p);				/* Session key */
	s->caps = gl32(p);			/* Server capabilities */
	*svrtime = gvtime(p);			/* fileserver time */
	s->tz = (short)gl16(p) * 60; /* TZ in mins, is signed (SNIA doc is wrong) */
	s->challen = g8(p);			/* Encryption key length */
	gl16(p);
	gmem(p, s->chal, s->challen);		/* Get the challenge */
	gstr(p, domain, domlen);		/* source domain */

	{		/* NetApp Filer seem not to report its called name */
		char *cn = emalloc9p(cnamlen);

		gstr(p, cn, cnamlen);		/* their name */
		if(strlen(cn) > 0)
			memcpy(cname, cn, cnamlen);
		free(cn);
	}

	if(s->caps & CAP_UNICODE)
		s->flags2 |= FL2_UNICODE;

	free(p);
	return 0;
}

int
CIFSsession(Session *s)
{
	char os[64], *q;
	Rune r;
	Pkt *p;
	enum {
		mycaps = CAP_UNICODE | CAP_LARGE_FILES | CAP_NT_SMBS |
			CAP_NT_FIND | CAP_STATUS32,
	};

	s->seqrun = 1;	/* activate the sequence number generation/checking */

	p = cifshdr(s, nil, SMB_COM_SESSION_SETUP_ANDX);
	p8(p, 0xFF);			/* No secondary command */
	p8(p, 0);			/* Reserved (must be zero) */
	pl16(p, 0);			/* Offset to next command */
	pl16(p, MTU);			/* my max buffer size */
	pl16(p, 1);			/* my max multiplexed pending requests */
	pl16(p, 0);			/* Virtual connection # */
	pl32(p, 0);			/* Session key (if vc != 0) */


	if((s->secmode & SECMODE_PW_ENCRYPT) == 0) {
		pl16(p, utflen(Sess->auth->resp[0])*2 + 2); /* passwd size */
		pl16(p, utflen(Sess->auth->resp[0])*2 + 2); /* passwd size (UPPER CASE) */
		pl32(p, 0);			/* Reserved */
		pl32(p, mycaps);
		pbytes(p);

		for(q = Sess->auth->resp[0]; *q; ){
			q += chartorune(&r, q);
			pl16(p, toupperrune(r));
		}
		pl16(p, 0);

		for(q = Sess->auth->resp[0]; *q; ){
			q += chartorune(&r, q);
			pl16(p, r);
		}
		pl16(p, 0);
	}else{
		pl16(p, Sess->auth->len[0]);	/* LM passwd size */
		pl16(p, Sess->auth->len[1]);	/* NTLM passwd size */
		pl32(p, 0);			/* Reserved  */
		pl32(p, mycaps);
		pbytes(p);

		pmem(p, Sess->auth->resp[0], Sess->auth->len[0]);
		pmem(p, Sess->auth->resp[1], Sess->auth->len[1]);
	}

	pstr(p, Sess->auth->user);	/* Account name */
	pstr(p, Sess->auth->windom);	/* Primary domain */
	pstr(p, "plan9");		/* Client OS */
	pstr(p, argv0);			/* Client LAN Manager type */

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	g8(p);				/* Reserved (0) */
	gl16(p);			/* Offset to next command wordcount */
	Sess->isguest = gl16(p) & 1;	/* logged in as guest */

	gl16(p);
	gl16(p);
	/* no security blob here - we don't understand extended security anyway */
	gstr(p, os, sizeof(os));
	s->remos = estrdup9p(os);

	free(p);
	return 0;
}


CIFStreeconnect(Session *s, char *cname, char *tree, Share *sp)
{
	int len;
	char *resp, *path;
	char zeros[24];
	Pkt *p;

	resp = Sess->auth->resp[0];
	len  = Sess->auth->len[0];
	if((s->secmode & SECMODE_USER) != SECMODE_USER){
		memset(zeros, 0, sizeof(zeros));
		resp = zeros;
		len = sizeof(zeros);
	}

	p = cifshdr(s, nil, SMB_COM_TREE_CONNECT_ANDX);
	p8(p, 0xFF);			/* Secondary command */
	p8(p, 0);			/* Reserved */
	pl16(p, 0);			/* Offset to next Word Count */
	pl16(p, 0);			/* Flags */

	if((s->secmode & SECMODE_PW_ENCRYPT) == 0){
		pl16(p, len+1);		/* password len, including null */
		pbytes(p);
		pascii(p, resp);
	}else{
		pl16(p, len);
		pbytes(p);
		pmem(p, resp, len);
	}

	path = smprint("//%s/%s", cname, tree);
	strupr(path);
	ppath(p, path);			/* path */
	free(path);

	pascii(p, "?????");	/* service type any (so we can do RAP calls) */

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}
	g8(p);				/* Secondary command */
	g8(p);				/* Reserved */
	gl16(p);			/* Offset to next command */
	sp->options = g8(p);		/* options supported */
	sp->tid = p->tid;		/* get received TID from packet header */
	free(p);
	return 0;
}

int
CIFSlogoff(Session *s)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, nil, SMB_COM_LOGOFF_ANDX);
	p8(p, 0xFF);			/* No ANDX command */
	p8(p, 0);			/* Reserved (must be zero) */
	pl16(p, 0);			/* offset ot ANDX */
	pbytes(p);
	rc = cifsrpc(p);

	free(p);
	return rc;
}

int
CIFStreedisconnect(Session *s, Share *sp)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_TREE_DISCONNECT);
	pbytes(p);
	rc = cifsrpc(p);

	free(p);
	return rc;
}


int
CIFSdeletefile(Session *s, Share *sp, char *name)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_DELETE);
	pl16(p, ATTR_HIDDEN|ATTR_SYSTEM);	/* search attributes */
	pbytes(p);
	p8(p, STR_ASCII);			/* buffer format */
	ppath(p, name);
	rc = cifsrpc(p);

	free(p);
	return rc;
}

int
CIFSdeletedirectory(Session *s, Share *sp, char *name)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_DELETE_DIRECTORY);
	pbytes(p);
	p8(p, STR_ASCII);		/* buffer format */
	ppath(p, name);
	rc = cifsrpc(p);

	free(p);
	return rc;
}

int
CIFScreatedirectory(Session *s, Share *sp, char *name)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_CREATE_DIRECTORY);
	pbytes(p);
	p8(p, STR_ASCII);
	ppath(p, name);
	rc = cifsrpc(p);

	free(p);
	return rc;
}

int
CIFSrename(Session *s, Share *sp, char *old, char *new)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_RENAME);
	pl16(p, ATTR_HIDDEN|ATTR_SYSTEM|ATTR_DIRECTORY); /* search attributes */
	pbytes(p);
	p8(p, STR_ASCII);
	ppath(p, old);
	p8(p, STR_ASCII);
	ppath(p, new);
	rc = cifsrpc(p);

	free(p);
	return rc;
}


/* for NT4/Win2k/XP */
int
CIFS_NT_opencreate(Session *s, Share *sp, char *name, int flags, int options,
	int attrs, int access, int share, int action, int *result, FInfo *fi)
{
	Pkt *p;
	int fh;

	p = cifshdr(s, sp, SMB_COM_NT_CREATE_ANDX);
	p8(p, 0xFF);			/* Secondary command */
	p8(p, 0);			/* Reserved */
	pl16(p, 0);			/* Offset to next command */
	p8(p, 0);			/* Reserved */
	pl16(p, utflen(name) *2);	/* file name len */
	pl32(p, flags);			/* Flags */
	pl32(p, 0);			/* fid of cwd, if relative path */
	pl32(p, access);		/* access desired */
	pl64(p, 0);			/* initial allocation size */
	pl32(p, attrs);			/* Extended attributes */
	pl32(p, share);			/* Share Access */
	pl32(p, action);		/* What to do on success/failure */
	pl32(p, options);		/* Options */
	pl32(p, SECURITY_IMPERSONATION); /* Impersonation level */
	p8(p, SECURITY_CONTEXT_TRACKING | SECURITY_EFFECTIVE_ONLY); /* security flags */
	pbytes(p);
	p8(p, 0);			/* FIXME: padding? */
	ppath(p, name);			/* filename */

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	memset(fi, 0, sizeof(FInfo));
	g8(p);				/* Secondary command */
	g8(p);				/* Reserved */
	gl16(p);			/* Offset to next command */
	g8(p);				/* oplock granted */
	fh = gl16(p);			/* FID for opened object */
	*result = gl32(p);		/* create action taken */
	gl64(p);			/* creation time */
	fi->accessed = gvtime(p);	/* last access time */
	fi->written = gvtime(p);	/* last written time */
	fi->changed = gvtime(p);	/* change time */
	fi->attribs = gl32(p);		/* extended attributes */
	gl64(p);			/* bytes allocated */
	fi->size = gl64(p);		/* file size */

	free(p);
	return fh;
}

/* for Win95/98/ME */
CIFS_SMB_opencreate(Session *s, Share *sp, char *name, int access,
	int attrs, int action, int *result)
{
	Pkt *p;
	int fh;

	p = cifshdr(s, sp, SMB_COM_OPEN_ANDX);
	p8(p, 0xFF);			/* Secondary command */
	p8(p, 0);			/* Reserved */
	pl16(p, 0);			/* Offset to next command */
	pl16(p, 0);			/* Flags (0 == no stat(2) info) */
	pl16(p, access);		/* desired access */
	pl16(p, ATTR_HIDDEN|ATTR_SYSTEM);/* search attributes */
	pl16(p, attrs);			/* file attribytes */
	pdatetime(p, 0);		/* creation time (0 == now) */
	pl16(p, action);		/* What to do on success/failure */
	pl32(p, 0);			/* allocation size */
	pl32(p, 0);			/* reserved */
	pl32(p, 0);			/* reserved */
	pbytes(p);
	ppath(p, name);			/* filename */

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	g8(p);				/* Secondary command */
	g8(p);				/* Reserved */
	gl16(p);			/* Offset to next command */
	fh = gl16(p);			/* FID for opened object */
	gl16(p);			/* extended attributes */
	gvtime(p);			/* last written time */
	gl32(p);			/* file size */
	gl16(p);			/* file type (disk/fifo/printer etc) */
	gl16(p);			/* device status (for fifos) */
	*result = gl16(p);		/* access granted */

	free(p);
	return fh;
}

vlong
CIFSwrite(Session *s, Share *sp, int fh, uvlong off, void *buf, vlong n)
{
	Pkt *p;
	vlong got;

	/* FIXME: Payload should be padded to long boundary */
	assert((n   & 0xffffffff00000000LL) == 0 || s->caps & CAP_LARGE_FILES);
	assert((off & 0xffffffff00000000LL) == 0 || s->caps & CAP_LARGE_FILES);
	assert(n < s->mtu - T2HDRLEN || s->caps & CAP_LARGE_WRITEX);

	p = cifshdr(s, sp, SMB_COM_WRITE_ANDX);
	p8(p, 0xFF);			/* Secondary command */
	p8(p, 0);			/* Reserved */
	pl16(p, 0);			/* Offset to next command */
	pl16(p, fh);			/* File handle */
	pl32(p, off & 0xffffffff);	/* LSBs of Offset */
	pl32(p, 0);			/* Reserved (0) */
	pl16(p, s->nocache);		/* Write mode (0 - write through) */
	pl16(p, 0);			/* Bytes remaining */
	pl16(p, n >> 16);		/* MSBs of length */
	pl16(p, n & 0xffffffff);	/* LSBs of length */
	pl16(p, T2HDRLEN);		/* Offset to data, in bytes */
	pl32(p, off >> 32);		/* MSBs of offset */
	pbytes(p);

	p->pos = p->buf +T2HDRLEN +NBHDRLEN;
	pmem(p, buf, n);		/* Data */

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	g8(p);				/* Secondary command */
	g8(p);				/* Reserved */
	gl16(p);			/* Offset to next command */
	got = gl16(p);			/* LSWs of bytes written */
	gl16(p);			/* remaining (space ?) */
	got |= (gl16(p) << 16);		/* MSWs of bytes written */

	free(p);
	return got;
}

vlong
CIFSread(Session *s, Share *sp, int fh, uvlong off, void *buf, vlong n,
	vlong minlen)
{
	int doff;
	vlong got;
	Pkt *p;

	assert((n   & 0xffffffff00000000LL) == 0 || s->caps & CAP_LARGE_FILES);
	assert((off & 0xffffffff00000000LL) == 0 || s->caps & CAP_LARGE_FILES);
	assert(n < s->mtu - T2HDRLEN || s->caps & CAP_LARGE_READX);

	p = cifshdr(s, sp, SMB_COM_READ_ANDX);
	p8(p, 0xFF);			/* Secondary command */
	p8(p, 0);			/* Reserved */
	pl16(p, 0);			/* Offset to next command */
	pl16(p, fh);			/* File handle */
	pl32(p, off & 0xffffffff);	/* Offset to beginning of write */
	pl16(p, n);			/* Maximum number of bytes to return */
	pl16(p, minlen);		/* Minimum number of bytes to return */
	pl32(p, (uint)n >> 16);		/* MSBs of maxlen */
	pl16(p, 0);			/* Bytes remaining to satisfy request */
	pl32(p, off >> 32);		/* MS 32 bits of offset */
	pbytes(p);

	if(cifsrpc(p) == -1){
		free(p);
		return -1;
	}

	g8(p);				/* Secondary command */
	g8(p);				/* Reserved */
	gl16(p);			/* Offset to next command */
	gl16(p);			/* Remaining */
	gl16(p);			/* Compression mode */
	gl16(p);			/* Reserved */
	got = gl16(p);			/* length */
	doff = gl16(p);			/* Offset from header to data */
	got |= gl16(p) << 16;

	p->pos = p->buf + doff + NBHDRLEN;

	gmem(p, buf, got);		 /* data */
	free(p);
	return got;
}

int
CIFSflush(Session *s, Share *sp, int fh)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_FLUSH);
	pl16(p, fh);			/* fid */
	pbytes(p);
	rc = cifsrpc(p);

	free(p);
	return rc;
}

/*
 * Setting the time of last write to -1 gives "now" if the file
 * was written and leaves it the same if the file wasn't written.
 */
int
CIFSclose(Session *s, Share *sp, int fh)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_CLOSE);
	pl16(p, fh);			/* fid */
	pl32(p, ~0L);			/* Time of last write (none) */
	pbytes(p);
	rc = cifsrpc(p);

	free(p);
	return rc;
}


int
CIFSfindclose2(Session *s, Share *sp, int sh)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_FIND_CLOSE2);
	pl16(p, sh);			/* sid */
	pbytes(p);
	rc = cifsrpc(p);

	free(p);
	return rc;
}


int
CIFSecho(Session *s)
{
	Pkt *p;
	int rc;

	p = cifshdr(s, nil, SMB_COM_ECHO);
	pl16(p, 1);				/* number of replies */
	pbytes(p);
	pascii(p, "abcdefghijklmnopqrstuvwxyz"); /* data */

	rc = cifsrpc(p);
	free(p);
	return rc;
}


int
CIFSsetinfo(Session *s, Share *sp, char *path, FInfo *fip)
{
	int rc;
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_SET_INFORMATION);
	pl16(p, fip->attribs);
	pl32(p, time(nil) - s->tz);	/* modified time */
	pl64(p, 0);			/* reserved */
	pl16(p, 0);			/* reserved */

	pbytes(p);
	p8(p, STR_ASCII);		/* buffer format */
	ppath(p, path);

	rc = cifsrpc(p);
	free(p);
	return rc;
}
