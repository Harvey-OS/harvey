#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

static Pkt *
t2hdr(Session *s, Share *sp, int cmd)
{
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_TRANSACTION2);

	p->tbase = pl16(p, 0);	/* 0  Total parameter bytes to be sent, filled later */
	pl16(p, 0);		/* 2  Total data bytes to be sent, filled later */
	pl16(p, 64);			/* 4  Max parameter to return */
	pl16(p, (MTU - T2HDRLEN)-64);	/* 6  Max data to return */
	p8(p, 0);			/* 8  Max setup count to return */
	p8(p, 0);			/* 9  Reserved */
	pl16(p, 0);			/* 10 Flags */
	pl32(p, 1000);			/* 12 Timeout (ms) */
	pl16(p, 0);			/* 16 Reserved */
	pl16(p, 0);			/* 18 Parameter count, filled later */
	pl16(p, 0);			/* 20 Parameter offset, filled later */
	pl16(p, 0);			/* 22 Data count, filled later */
	pl16(p, 0);			/* 24 Data offset, filled later */
	p8(p, 1);			/* 26 Setup count (in words) */
	p8(p, 0);			/* 27 Reserved */
	pl16(p, cmd);			/* setup[0] */
	pbytes(p);
	p8(p, 0);			/* padding ??!?!? */

	return p;
}

static void
pt2param(Pkt *p)
{
	uchar *pos = p->pos;

	assert(p->tbase != 0);
	p->pos = p->tbase + 20;
	pl16(p, (pos - p->buf) - NBHDRLEN); /* param offset */

	p->tparam = p->pos = pos;
}

static void
pt2data(Pkt *p)
{
	uchar *pos = p->pos;

	assert(p->tbase != 0);
	assert(p->tparam != 0);

	p->pos = p->tbase +0;
	pl16(p, pos - p->tparam);		/* total param count */

	p->pos = p->tbase +18;
	pl16(p, pos - p->tparam);		/* param count */

	p->pos = p->tbase +24;
	pl16(p, (pos - p->buf) - NBHDRLEN);	/* data offset */

	p->tdata = p->pos = pos;
}

static int
t2rpc(Pkt *p)
{
	int got;
	uchar *pos;

	assert(p->tbase != 0);
	assert(p->tdata != 0);

	pos = p->pos;

	p->pos = p->tbase +2;
	pl16(p, pos - p->tdata);		/* total data count */

	p->pos = p->tbase +22;
	pl16(p, pos - p->tdata);		/* data count */

	p->pos = pos;
	if((got = cifsrpc(p)) == -1)
		return -1;

	gl16(p);			/* Total parameter count */
	gl16(p);			/* Total data count */
	gl16(p);			/* Reserved */
	gl16(p);			/* Parameter count in this buffer */
	p->tparam = p->buf +NBHDRLEN +gl16(p); /* Parameter offset */
	gl16(p);			/* Parameter displacement */
	gl16(p);			/* Data count (this buffer); */
	p->tdata = p->buf +NBHDRLEN +gl16(p); /* Data offset */
	gl16(p);			/* Data displacement */
	g8(p);				/* Setup count */
	g8(p);				/* Reserved */

	return got;
}

static void
gt2param(Pkt *p)
{
	p->pos = p->tparam;
}

static void
gt2data(Pkt *p)
{
	p->pos = p->tdata;
}


int
T2findfirst(Session *s, Share *sp, int slots, char *path, int *got,
	long *resume, FInfo *fip)
{
	int pktlen, i, n, sh;
	uchar *next;
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_FIND_FIRST2);
	p8(p, 'D');			/* OS/2 */
	p8(p, ' ');			/* OS/2 */

	pt2param(p);
	pl16(p, ATTR_HIDDEN|ATTR_SYSTEM|ATTR_DIRECTORY); /* Search attributes */
	pl16(p, slots);			/* Search count */
	pl16(p, CIFS_SEARCH_RETURN_RESUME); /* Flags */
	pl16(p, SMB_FIND_FILE_FULL_DIRECTORY_INFO); /* Information level */
	pl32(p, 0);			/* SearchStorage type (?) */
	ppath(p, path);			/* path */

	pt2data(p);
	if((pktlen = t2rpc(p)) == -1){
		free(p);
		return -1;
	}

	s->lastfind = nsec();
	gt2param(p);

	sh = gl16(p);			/* Sid (search handle) */
	*got = gl16(p);			/* number of slots received */
	gl16(p);			/* End of search flag */
	gl16(p);			/* Offset into EA list if EA error */
	gl16(p);			/* Offset into data to file name of last entry */

	gt2data(p);
	memset(fip, 0, slots * sizeof(FInfo));
	for(i = 0; i < *got; i++){
		next = p->pos;
		next += gl32(p);	/* offset to next entry */
		/*
		 * bug in Windows - somtimes it lies about how many
		 * directory entries it has put in the packet
		 */
		if(next - p->buf > pktlen){
			*got = i;
			break;
		}

		*resume = gl32(p);		/* resume key for search */
		fip[i].created = gvtime(p);	/* creation time */
		fip[i].accessed = gvtime(p);	/* last access time */
		fip[i].written = gvtime(p);	/* last written time */
		fip[i].changed = gvtime(p);	/* change time */
		fip[i].size = gl64(p);		/* file size */
		gl64(p);			/* bytes allocated */
		fip[i].attribs = gl32(p);	/* extended attributes */
		n = gl32(p);			/* name length */
		gl32(p);			/* EA size */
		gstr(p, fip[i].name, n); 	/* name */
		p->pos = next;
	}

	free(p);
	return sh;

}

int
T2findnext(Session *s, Share *sp, int slots, char *path, int *got,
	long *resume, FInfo *fip, int sh)
{
	Pkt *p;
	int i, n;
	uchar *next;

	/*
	 * So I believe from comp.protocols.smb if you send
	 * TRANS2_FIND_NEXT2 requests too quickly to windows 95, it can
	 * get confused and fail to reply, so we slow up a bit in these
	 * circumstances.
	 */
	if(!(s->caps & CAP_NT_SMBS) && nsec() - s->lastfind < 200000000LL)
		sleep(200);

	p = t2hdr(s, sp, TRANS2_FIND_NEXT2);
	p8(p, 'D');			/* OS/2 */
	p8(p, ' ');			/* OS/2 */

	pt2param(p);
	pl16(p, sh);				/* search handle */
	pl16(p, slots);				/* Search count */
	pl16(p, SMB_FIND_FILE_FULL_DIRECTORY_INFO); /* Information level */
	pl32(p, *resume);			/* resume key */
	pl16(p, CIFS_SEARCH_CONTINUE_FROM_LAST); /* Flags */
	ppath(p, path);				/* file+path to resume */

	pt2data(p);
	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}

	s->lastfind = nsec();

	gt2param(p);
	*got = gl16(p);		/* number of slots received */
	gl16(p);		/* End of search flag */
	gl16(p);		/* Offset into EA list if EA error */
	gl16(p);		/* Offset into data to file name of last entry */

	gt2data(p);
	memset(fip, 0, slots * sizeof(FInfo));
	for(i = 0; i < *got; i++){
		next = p->pos;
		next += gl32(p);		/* offset to next entry */
		*resume = gl32(p);		/* resume key for search */
		fip[i].created = gvtime(p);	/* creation time */
		fip[i].accessed = gvtime(p);	/* last access time */
		fip[i].written = gvtime(p);	/* last written time */
		fip[i].changed = gvtime(p);	/* change time */
		fip[i].size = gl64(p);		/* file size */
		gl64(p);			/* bytes allocated */
		fip[i].attribs = gl32(p);	/* extended attributes */
		n = gl32(p);			/* name length */
		gl32(p);			/* EA size */
		gstr(p, fip[i].name, n); 	/* name */
		p->pos = next;
	}
	free(p);
	return 0;
}


/* supported by 2k/XP/NT4 */
int
T2queryall(Session *s, Share *sp, char *path, FInfo *fip)
{
	int n;
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_QUERY_PATH_INFORMATION);
	pt2param(p);
	pl16(p, SMB_QUERY_FILE_ALL_INFO); /* Information level	 */
	pl32(p, 0);			/* reserved */
	ppath(p, path);			/* path */

	pt2data(p);
	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}
	gt2data(p);

	/*
	 * The layout of this struct is wrong in the SINA
	 * document, this layout gained by inspection.
	 */
	memset(fip, 0, sizeof(FInfo));
	fip->created = gvtime(p);	/* creation time */
	fip->accessed = gvtime(p);	/* last access time */
	fip->written = gvtime(p);	/* last written time */
	fip->changed = gvtime(p);	/* change time */
	fip->attribs = gl32(p);		/* attributes */
	gl32(p);			/* reserved */
	gl64(p);			/* bytes allocated */
	fip->size = gl64(p);		/* file size */
	gl32(p);			/* number of hard links */
	g8(p);				/* delete pending */
	g8(p);				/* is a directory */
	gl16(p);			/* reserved */
	gl32(p);			/* EA size */

	n = gl32(p);
	if(n >= sizeof fip->name)
		n = sizeof fip->name - 1;
	gstr(p, fip->name, n);

	free(p);
	return 0;
}

/* supported by 95/98/ME */
int
T2querystandard(Session *s, Share *sp, char *path, FInfo *fip)
{
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_QUERY_PATH_INFORMATION);
	pt2param(p);
	pl16(p, SMB_INFO_STANDARD);	/* Information level */
	pl32(p, 0);			/* reserved */
	ppath(p, path);			/* path */

	pt2data(p);
	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}
	gt2data(p);
	memset(fip, 0, sizeof(FInfo));
	fip->created = gdatetime(p);	/* creation time */
	fip->accessed = gdatetime(p);	/* last access time */
	fip->written = gdatetime(p);	/* last written time */
	fip->changed = fip->written;	/* change time */
	fip->size = gl32(p);		/* file size */
	gl32(p);			/* bytes allocated */
	fip->attribs = gl16(p);		/* attributes */
	gl32(p);			/* EA size */

	free(p);
	return 0;
}

int
T2setpathinfo(Session *s, Share *sp, char *path, FInfo *fip)
{
	int rc;
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_SET_PATH_INFORMATION);
	pt2param(p);
	pl16(p, SMB_INFO_STANDARD);	/* Information level */
	pl32(p, 0);			/* reserved */
	ppath(p, path);			/* path */

	pt2data(p);
	pdatetime(p, fip->created);	/* created */
	pdatetime(p, fip->accessed);	/* accessed */
	pdatetime(p, fip->written);	/* written */
	pl32(p, fip->size);		/* size */
	pl32(p, 0);			/* allocated */
	pl16(p, fip->attribs);		/* attributes */
	pl32(p, 0);			/* EA size */
	pl16(p, 0);			/* reserved */

	rc = t2rpc(p);
	free(p);
	return rc;

}

int
T2setfilelength(Session *s, Share *sp, int fh, FInfo *fip) /* FIXME: maybe broken, needs testing */
{
	int rc;
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_SET_FILE_INFORMATION);
	pt2param(p);
	pl16(p, fh);			/* file handle */
	pl16(p, SMB_SET_FILE_END_OF_FILE_INFO); /* Information level */
	pl16(p, 0);			/* reserved */

	pt2data(p);
	pl64(p, fip->size);
	pl32(p, 0);			/* padding ?! */
	pl16(p, 0);

	rc = t2rpc(p);
	free(p);
	return rc;
}


int
T2fsvolumeinfo(Session *s, Share *sp, long *created, long *serialno,
	char *label, int labellen)
{
	Pkt *p;
	long ct, sn, n;

	p = t2hdr(s, sp, TRANS2_QUERY_FS_INFORMATION);
	pt2param(p);
	pl16(p, SMB_QUERY_FS_VOLUME_INFO);	/* Information level */

	pt2data(p);

	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}

	gt2data(p);
	ct = gvtime(p);			/* creation time */
	sn = gl32(p);			/* serial number */
	n = gl32(p);			/* label name length */
	g8(p);				/* reserved */
	g8(p);				/* reserved */

	memset(label, 0, labellen);
	if(n < labellen && n > 0)
		gstr(p, label, n);	/* file system label */
	if(created)
		*created = ct;
	if(serialno)
		*serialno = sn;

	free(p);
	return 0;
}

int
T2fssizeinfo(Session *s, Share *sp, uvlong *total, uvlong *unused)
{
	Pkt *p;
	uvlong t, f, n, b;

	p = t2hdr(s, sp, TRANS2_QUERY_FS_INFORMATION);
	pt2param(p);
	pl16(p, SMB_QUERY_FS_SIZE_INFO);	/* Information level */

	pt2data(p);

	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}

	gt2data(p);
	t = gl64(p);		/* total blocks */
	f = gl64(p);		/* free blocks */
	n = gl32(p);		/* sectors per block */
	b = gl32(p);		/* bytes per sector */

	if(free)
		*unused = f * n * b;
	if(total)
		*total = t * n * b;

	free(p);
	return 0;
}

int
T2getdfsreferral(Session *s, Share *sp, char *path, int *gflags, int *used,
	Refer *re, int nent)
{
	int i, vers, nret, len;
	char tmp[1024];
	uchar *base;
	Pkt *p;

	p = t2hdr(s, sp, TRANS2_GET_DFS_REFERRAL);
	pt2param(p);
	pl16(p, 3); /* max info level we understand, must be >= 3 for domain requests */
	ppath(p, path);

	pt2data(p);

	if(t2rpc(p) == -1){
		free(p);
		return -1;
	}

	memset(re, 0, sizeof *re * nent);
	gt2data(p);

	*used = gl16(p) / 2;	/* length used (/2 as Windows counts in runes) */
	nret = gl16(p);		/* number of referrals returned */
	*gflags = gl32(p);	/* global flags */

	for(i = 0; i < nret && i < nent && i < 16; i++){
		base = p->pos;
		vers = gl16(p);		/* version of records */
		len = gl16(p);		/* length of records */
		re[i].type = gl16(p);	/* server type */
		re[i].flags = gl16(p);	/* referal flags */
		switch(vers){
		case 1:
			re[i].prox = 0;	/* nearby */
			re[i].ttl = 5*60;	/* 5 mins */
			gstr(p, tmp, sizeof tmp);
			re[i].addr = estrdup9p(tmp);
			re[i].path = estrdup9p(tmp);
			break;
		case 2:
			re[i].prox = gl32(p);	/* not implemented in v2 */
			re[i].ttl = gl32(p);
			goff(p, base, re[i].path, sizeof tmp);
			re[i].path = estrdup9p(tmp);
			goff(p, base, re[i].path, sizeof tmp);/* spurious 8.3 path */
			goff(p, base, tmp, sizeof tmp);
			re[i].addr = estrdup9p(tmp);
			break;
		case 3:
			if(re[i].flags & DFS_REFERAL_LIST){
				re[i].prox = 0;
				re[i].ttl = gl32(p);
				goff(p, base, tmp, sizeof tmp);
				re[i].path = estrdup9p(tmp);
				gl16(p);
				goff(p, base, tmp, sizeof tmp);
				re[i].addr = estrdup9p(tmp);
			}
			else{
				re[i].prox = 0;
				re[i].ttl = gl32(p);
				goff(p, base, tmp, sizeof tmp);
				re[i].path = estrdup9p(tmp);
				gl16(p);	/* spurious 8.3 path */
				goff(p, base, tmp, sizeof tmp);
				re[i].addr = estrdup9p(tmp);
				gl16(p);	/* GUID (historic) */
			}
			break;
		default:
			/*
			 * this should never happen as we specify our maximum
			 * understood level in the request (above)
			 */
			fprint(2, "%d - unsupported DFS infolevel\n", vers);
			re[i].path = estrdup9p(tmp);
			re[i].addr = estrdup9p(tmp);
			break;
		}
		p->pos = base+len;
	}

	free(p);
	return i;
}
