#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

static Pkt *
tnthdr(Session *s, Share *sp, int cmd)
{
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_NT_TRANSACT);
	p->tbase = p8(p, 0);		/*  0  Max setup count to return */
	pl16(p, 0);			/*  1  reserved */
	pl32(p, 0);			/*  3  Total parameter count */
	pl32(p, 0);			/*  7  Total data count */
	pl32(p, 64);			/* 11  Max parameter count to return */
	pl32(p, (MTU - T2HDRLEN)-64);	/* 15  Max data count to return */
	pl32(p, 0);			/* 19  Parameter count (in this buffer) */
	pl32(p, 0);			/* 23  Offset to parameters (in this buffer) */
	pl32(p, 0);			/* 27  Count of data  in this buffer */
	pl32(p, 0);			/* 31  Offset to data in this buffer */
	p8(p, 1);			/* 35  Count of setup words */
	pl16(p, cmd);			/* 37  setup[0] */
	pl16(p, 0);			/* padding ??!?!? */
	pbytes(p);
	return p;
}

static void
ptntparam(Pkt *p)
{
	uchar *pos = p->pos;
	assert(p->tbase != 0);

	p->pos = p->tbase +23;
	pl32(p, (pos - p->buf) - NBHDRLEN); /* param offset */

	p->tparam = p->pos = pos;
}

static void
ptntdata(Pkt *p)
{
	uchar *pos = p->pos;
	assert(p->tbase != 0);
	assert(p->tparam != 0);

	p->pos = p->tbase +3;
	pl32(p, pos - p->tparam);		/* total param count */

	p->pos = p->tbase +19;
	pl32(p, pos - p->tparam);		/* param count */

	p->pos = p->tbase +31;
	pl32(p, (pos - p->buf) - NBHDRLEN);	/* data offset */
	p->tdata = p->pos = pos;
}

static int
tntrpc(Pkt *p)
{
	int got;
	uchar *pos;
	assert(p->tbase != 0);
	assert(p->tdata != 0);

	pos = p->pos;

	p->pos = p->tbase +7;
	pl32(p, pos - p->tdata);		/* total data count */

	p->pos = p->tbase +27;
	pl32(p, pos - p->tdata);		/* data count */

	p->pos = pos;
	if((got = cifsrpc(p)) == -1)
		return -1;

	g8(p);				/* Reserved */
	g8(p);				/* Reserved */
	g8(p);				/* Reserved */
	gl32(p);			/* Total parameter count */
	gl32(p);			/* Total data count */
	gl32(p);			/* Parameter count in this buffer */
	p->tparam = p->buf +NBHDRLEN +gl32(p); /* Parameter offset */
	gl32(p);			/* Parameter displacement */
	gl32(p);			/* Data count (this buffer); */
	p->tdata = p->buf +NBHDRLEN +gl32(p); /* Data offset */
	gl32(p);			/* Data displacement */
	g8(p);				/* Setup count */
 	gl16(p);			/* padding ???  */

	return got;
}

static void
gtntparam(Pkt *p)
{
	p->pos = p->tparam;
}

static void
gtntdata(Pkt *p)
{
	p->pos = p->tdata;
}


int
TNTquerysecurity(Session *s, Share *sp, int fh, char **usid, char **gsid)
{
	Pkt *p;
	uchar *base;
	Fmt fmt, *f = &fmt;
	int n, i, off2owner, off2group;

	p = tnthdr(s, sp, NT_TRANSACT_QUERY_SECURITY_DESC);
	ptntparam(p);

	pl16(p, fh); 		/* File handle */
	pl16(p, 0); 		/* Reserved */
	pl32(p, QUERY_OWNER_SECURITY_INFORMATION |
		QUERY_GROUP_SECURITY_INFORMATION);

	ptntdata(p);

	if(tntrpc(p) == -1){
		free(p);
		return -1;
	}

	gtntdata(p);

	base = p->pos;
	gl16(p);			/* revision */
	gl16(p);			/* type */
	off2owner = gl32(p);		/* offset to owner */
	off2group = gl32(p);		/* offset to group */
	gl32(p);
	gl32(p);

	if(off2owner){
		p->pos = base +  off2owner;
		fmtstrinit(f);
		fmtprint(f, "S-%ud", g8(p));	/* revision */
		n = g8(p);			/* num auth */
		fmtprint(f, "-%llud", gb48(p));	/* authority */
		for(i = 0; i < n; i++)
			fmtprint(f, "-%ud", gl32(p));	/* sub-authorities */
		*usid = fmtstrflush(f);
	}

	if(off2group){
		p->pos = base +  off2group;
		fmtstrinit(f);
		fmtprint(f, "S-%ud", g8(p));	/* revision */
		n = g8(p);			/* num auth */
		fmtprint(f, "-%llud", gb48(p));	/* authority */
		for(i = 0; i < n; i++)
			fmtprint(f, "-%ud", gl32(p));	/* sub-authorities */
		*gsid = fmtstrflush(f);
	}
	free(p);
	return 0;
}
