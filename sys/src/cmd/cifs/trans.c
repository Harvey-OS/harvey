#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"
#include "remsmb.h"
#include "apinums.h"

static Pkt *
thdr(Session *s, Share *sp)
{
	Pkt *p;

	p = cifshdr(s, sp, SMB_COM_TRANSACTION);
	p->tbase = pl16(p, 0);	/* 0  Total parameter bytes to be sent, filled later */
	pl16(p, 0);		/* 2  Total data bytes to be sent, filled later */
	pl16(p, 64);			/* 4  Max parameter to return */
	pl16(p, MTU - T2HDRLEN - 128);	/* 6  Max data to return */
	pl16(p, 1);			/* 8  Max setup count to return */
	pl16(p, 0);			/* 10 Flags */
	pl32(p, 1000);			/* 12 Timeout (ms) */
	pl16(p, 0);			/* 16 Reserved */
	pl16(p, 0);			/* 18 Parameter count, filled later */
	pl16(p, 0);			/* 20 Parameter offset, filled later */
	pl16(p, 0);			/* 22 Data count, filled later */
	pl16(p, 0);			/* 24 Data offset, filled later */
	pl16(p, 0);			/* 26 Setup count (in words) */
	pbytes(p);			/* end of cifs words section */
	return p;
}

static void
ptparam(Pkt *p)
{
	uchar *pos;

	if(((p->pos - p->tbase) % 2) != 0)
		p8(p, 0);			/* pad to word boundry */
	pos = p->pos;
	p->pos = p->tbase + 20;
	pl16(p, pos - p->buf - NBHDRLEN);	/* param offset */
	p->tparam = p->pos = pos;
}

static void
ptdata(Pkt *p)
{
	uchar *pos = p->pos;

	assert(p->tparam != 0);
	if(((p->pos - p->tbase) % 2) != 0)
		p8(p, 0);		/* pad to word boundry */

	p->pos = p->tbase + 0;
	pl16(p, pos - p->tparam);	/* total param count */

	p->pos = p->tbase + 18;
	pl16(p, pos - p->tparam);	/* param count */

	p->pos = p->tbase + 24;
	pl16(p, pos - p->buf - NBHDRLEN); /* data offset */

	p->tdata = p->pos = pos;
}

static int
trpc(Pkt *p)
{
	int got;
	uchar *pos = p->pos;

	assert(p->tbase != 0);
	assert(p->tdata != 0);

	p->pos = p->tbase + 2;
	pl16(p, pos - p->tdata);	/* total data count */

	p->pos = p->tbase + 22;
	pl16(p, pos - p->tdata);	/* data count */

	p->pos = pos;
	if((got = cifsrpc(p)) == -1)
		return -1;

	gl16(p);			/* Total parameter count */
	gl16(p);			/* Total data count */
	gl16(p);			/* Reserved */
	gl16(p);			/* Parameter count in this buffer */
	p->tparam = p->buf + NBHDRLEN + gl16(p); /* Parameter offset */
	gl16(p);			/* Parameter displacement */
	gl16(p);			/* Data count (this buffer); */
	p->tdata = p->buf + NBHDRLEN + gl16(p); /* Data offset */
	gl16(p);			/* Data displacement */
	g8(p);				/* Setup count */
	g8(p);				/* Reserved */
	return got;
}

static void
gtparam(Pkt *p)
{
	p->pos = p->tparam;
}

static void
gtdata(Pkt *p)
{
	p->pos = p->tdata;
}


int
RAPshareenum(Session *s, Share *sp, Share **ent)
{
	int ngot = 0, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Share *q;

	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WShareEnum);
	pascii(p, REMSmb_NetShareEnum_P);	/* request descriptor */
	pascii(p, REMSmb_share_info_0);		/* reply descriptor */
	pl16(p, 0);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	if(ngot == 0){
		*ent = emalloc9p(sizeof(Share) * navail);
		memset(*ent, 0, sizeof(Share) * navail);
	}

	q = *ent + ngot;
	for (; ngot < navail && nret--; ngot++){
		gmem(p, tmp, 13); 		/* name */
		tmp[13] = 0;
		q->name = estrdup9p(tmp);
		q++;
	}

	if(ngot < navail)
		fprint(2, "%s: %d/%d - share list incomplete\n", argv0, ngot, navail);

	free(p);
	return ngot;
}


int
RAPshareinfo(Session *s, Share *sp, char *share, Shareinfo2 *si2p)
{
	int conv, err;
	char tmp[1024];
	Pkt *p;

	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");

	ptparam(p);
	pl16(p, API_WShareGetInfo);
	pascii(p, REMSmb_NetShareGetInfo_P);	/* request descriptor */
	pascii(p, REMSmb_share_info_2);		/* reply descriptor */
	pascii(p, share);
	pl16(p, 1);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */

	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	gl16(p);				/* number of entries returned */
	gl16(p);				/* number of entries available */

	if(err){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	memset(si2p, 0, sizeof(Shareinfo2));

	gmem(p, tmp, 13);
	tmp[13] = 0;
	g8(p);					/* padding */
	si2p->name = estrdup9p(tmp);
	si2p->type = gl16(p);
	gconv(p, conv, tmp, sizeof tmp);
	si2p->comment = estrdup9p(tmp);
	gl16(p);				/* comment offset high (unused) */
	si2p->perms = gl16(p);
	si2p->maxusrs = gl16(p);
	si2p->activeusrs = gl16(p);
	gconv(p, conv, tmp, sizeof tmp);
	si2p->path = estrdup9p(tmp);
	gl16(p);				/* path offset high (unused) */
	gmem(p, tmp, 9);
	tmp[9] = 0;
	si2p->passwd = estrdup9p(tmp);

	free(p);
	return 0;
}

/*
 * Tried to split sessionenum into two passes, one getting the names
 * of the connected workstations and the other collecting the detailed info,
 * however API_WSessionGetInfo doesn't seem to work agains win2k3 for infolevel
 * ten and infolevel one and two are priviledged calls.  This means this code
 * will work for small numbers of sessions agains win2k3 and fail for samba 3.0
 * as it supports info levels zero and two only.
 */
int
RAPsessionenum(Session *s, Share *sp, Sessinfo **sip)
{
	int ngot = 0, conv, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Sessinfo *q;

	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WSessionEnum);
	pascii(p, REMSmb_NetSessionEnum_P);	/* request descriptor */
	pascii(p, REMSmb_session_info_10);	/* reply descriptor */
	pl16(p, 10);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	if(ngot == 0){
		*sip = emalloc9p(sizeof(Sessinfo) * navail);
		memset(*sip, 0, sizeof(Sessinfo) * navail);
	}

	q = *sip + ngot;
	while(nret-- != 0){
		gconv(p, conv, tmp, sizeof tmp);
		q->wrkstn = estrdup9p(tmp);
		gconv(p, conv, tmp, sizeof tmp);
		q->user = estrdup9p(tmp);
		q->sesstime = gl32(p);
		q->idletime = gl32(p);
		ngot++;
		q++;
	}
	if(ngot < navail)
		fprint(2, "warning: %d/%d - session list incomplete\n", ngot, navail);
	free(p);
	return ngot;
}


int
RAPgroupenum(Session *s, Share *sp, Namelist **nlp)
{
	int ngot, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Namelist *q;

	ngot = 0;
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WGroupEnum);
	pascii(p, REMSmb_NetGroupEnum_P);	/* request descriptor */
	pascii(p, REMSmb_group_info_0);		/* reply descriptor */
	pl16(p, 0);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	*nlp = emalloc9p(sizeof(Namelist) * navail);
	memset(*nlp, 0, sizeof(Namelist) * navail);

	q = *nlp + ngot;
	while(ngot < navail && nret--){
 		gmem(p, tmp, 21);
		tmp[21] = 0;
		q->name = estrdup9p(tmp);
		ngot++;
		q++;
		if(p->pos >= p->eop)		/* Windows seems to lie somtimes */
			break;
	}
	free(p);
	return ngot;
}


int
RAPgroupusers(Session *s, Share *sp, char *group, Namelist **nlp)
{
	int ngot, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Namelist *q;

	ngot = 0;
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WGroupGetUsers);
	pascii(p, REMSmb_NetGroupGetUsers_P);	/* request descriptor */
	pascii(p, REMSmb_user_info_0);		/* reply descriptor */
	pascii(p, group);			/* group name for list */
	pl16(p, 0);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	*nlp = emalloc9p(sizeof(Namelist) * navail);
	memset(*nlp, 0, sizeof(Namelist) * navail);

	q = *nlp + ngot;
	while(ngot < navail && nret--){
 		gmem(p, tmp, 21);
		tmp[21] = 0;
		q->name = estrdup9p(tmp);
		ngot++;
		q++;
		if(p->pos >= p->eop)		/* Windows seems to lie somtimes */
			break;
	}
	free(p);
	return ngot;
}

int
RAPuserenum(Session *s, Share *sp, Namelist **nlp)
{
	int ngot, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Namelist *q;

	ngot = 0;
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WUserEnum);
	pascii(p, REMSmb_NetUserEnum_P);	/* request descriptor */
	pascii(p, REMSmb_user_info_0);		/* reply descriptor */
	pl16(p, 0);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	*nlp = emalloc9p(sizeof(Namelist) * navail);
	memset(*nlp, 0, sizeof(Namelist) * navail);

	q = *nlp + ngot;
	while(ngot < navail && nret--){
 		gmem(p, tmp, 21);
		tmp[21] = 0;
		q->name = estrdup9p(tmp);
		ngot++;
		q++;
		if(p->pos >= p->eop)		/* Windows seems to lie somtimes */
			break;
	}
	free(p);
	return ngot;
}

int
RAPuserenum2(Session *s, Share *sp, Namelist **nlp)
{
	int ngot, resume, err, navail, nret;
	char tmp[1024];
	Pkt *p;
	Namelist *q;

	ngot = 0;
	resume = 0;
more:
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WUserEnum2);
	pascii(p, REMSmb_NetUserEnum2_P);	/* request descriptor */
	pascii(p, REMSmb_user_info_0);		/* reply descriptor */
	pl16(p, 0);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	pl32(p, resume);			/* resume key to allow multiple fetches */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	gl16(p);				/* rx buffer offset */
	resume = gl32(p);			/* resume key returned */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	if(ngot == 0){
		*nlp = emalloc9p(sizeof(Namelist) * navail);
		memset(*nlp, 0, sizeof(Namelist) * navail);
	}
	q = *nlp + ngot;
	while(ngot < navail && nret--){
 		gmem(p, tmp, 21);
		tmp[21] = 0;
		q->name = estrdup9p(tmp);
		ngot++;
		q++;
		if(p->pos >= p->eop)		/* Windows seems to lie somtimes */
			break;
	}
	free(p);
	if(ngot < navail)
		goto more;
	return ngot;
}

int
RAPuserinfo(Session *s, Share *sp, char *user, Userinfo *uip)
{
	int conv, err;
	char tmp[1024];
	Pkt *p;

	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");
	ptparam(p);

	pl16(p, API_WUserGetInfo);
	pascii(p, REMSmb_NetUserGetInfo_P);	/* request descriptor */
	pascii(p, REMSmb_user_info_10);		/* reply descriptor */
	pascii(p, user);			/* username */
	pl16(p, 10);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	gl16(p);				/* number of entries returned */
	gl16(p);				/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

 	gmem(p, tmp, 21);
	tmp[21] = 0;
	uip->user = estrdup9p(tmp);
	g8(p);				/* padding */
	gconv(p, conv, tmp, sizeof tmp);
	uip->comment = estrdup9p(tmp);
	gconv(p, conv, tmp, sizeof tmp);
	uip->user_comment = estrdup9p(tmp);
	gconv(p, conv, tmp, sizeof tmp);
	uip->fullname = estrdup9p(tmp);

	free(p);
	return 0;
}

/*
 * This works agains win2k3 but fails
 * against XP with the undocumented error 71/0x47
 */
int
RAPServerenum2(Session *s, Share *sp, char *workgroup, int type, int *more,
	Serverinfo **si)
{
	int ngot = 0, conv, err, nret, navail;
	char tmp[1024];
	Pkt *p;
	Serverinfo *q;

	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");

	ptparam(p);
	pl16(p, API_NetServerEnum2);
	pascii(p, REMSmb_NetServerEnum2_P);	/* request descriptor */
	pascii(p, REMSmb_server_info_1);	/* reply descriptor */
	pl16(p, 1);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	pl32(p, type);
	pascii(p, workgroup);

	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	*si = emalloc9p(sizeof(Serverinfo) * navail);
	memset(*si, 0, sizeof(Serverinfo) * navail);

	q = *si;
	for (; nret-- != 0 && ngot < navail; ngot++){
		gmem(p, tmp, 16);
		tmp[16] = 0;
		q->name = estrdup9p(tmp);
		q->major = g8(p);
		q->minor = g8(p);
		q->type = gl32(p);
		gconv(p, conv, tmp, sizeof tmp);
		q->comment = estrdup9p(tmp);
		q++;
	}
	free(p);
	*more = err == RAP_ERR_MOREINFO;
	return ngot;
}

int
RAPServerenum3(Session *s, Share *sp, char *workgroup, int type, int last,
	Serverinfo *si)
{
	int conv, err, ngot, nret, navail;
	char *first, tmp[1024];
	Pkt *p;
	Serverinfo *q;

	ngot = last +1;
	first = si[last].name;
more:
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");

	ptparam(p);
	pl16(p, API_NetServerEnum3);
	pascii(p, REMSmb_NetServerEnum3_P);	/* request descriptor */
	pascii(p, REMSmb_server_info_1);	/* reply descriptor */
	pl16(p, 1);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	pl32(p, type);
	pascii(p, workgroup);
	pascii(p, first);

	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	if(nret < 2){				/* paranoia */
		free(p);
		return ngot;
	}

	q = si+ngot;
	while(nret-- != 0 && ngot < navail){
		gmem(p, tmp, 16);
		tmp[16] = 0;
		q->name = estrdup9p(tmp);
		q->major = g8(p);
		q->minor = g8(p);
		q->type = gl32(p);
		gconv(p, conv, tmp, sizeof tmp);
		tmp[sizeof tmp - 1] = 0;
		q->comment = estrdup9p(tmp);
		if(strcmp(first, tmp) == 0){ /* 1st one thru _may_ be a repeat */
			free(q->name);
			free(q->comment);
			continue;
		}
		ngot++;
		q++;
	}
	free(p);
	if(ngot < navail)
		goto more;
	return ngot;
}

/* Only the Administrator has permission to do this */
int
RAPFileenum2(Session *s, Share *sp, char *user, char *path, Fileinfo **fip)
{
	int conv, err, ngot, resume, nret, navail;
	char tmp[1024];
	Pkt *p;
	Fileinfo *q;

	ngot = 0;
	resume = 0;
more:
	p = thdr(s, sp);
	pstr(p, "\\PIPE\\LANMAN");

	ptparam(p);
	pl16(p, API_WFileEnum2);
	pascii(p, REMSmb_NetFileEnum2_P);	/* request descriptor */
	pascii(p, REMSmb_file_info_1);		/* reply descriptor */
	pascii(p, path);
	pascii(p, user);
	pl16(p, 1);				/* detail level */
	pl16(p, MTU - 200);			/* receive buffer length */
	pl32(p, resume);			/* resume key */
/* FIXME: maybe the padding and resume key are the wrong way around? */
	pl32(p, 0);				/* padding ? */

	ptdata(p);

	if(trpc(p) == -1){
		free(p);
		return -1;
	}

	gtparam(p);
	err = gl16(p);				/* error code */
	conv = gl16(p);				/* rx buffer offset */
	resume = gl32(p);			/* resume key returned */
	nret = gl16(p);				/* number of entries returned */
	navail = gl16(p);			/* number of entries available */

	if(err && err != RAP_ERR_MOREINFO){
		werrstr("%s", raperrstr(err));
		free(p);
		return -1;
	}

	if(nret < 2){				/* paranoia */
		free(p);
		return ngot;
	}

	if(ngot == 0){
		*fip = emalloc9p(sizeof(Fileinfo) * navail);
		memset(*fip, 0, sizeof(Fileinfo) * navail);
	}
	q = *fip + ngot;
	for(; nret-- && ngot < navail; ngot++){
		q->ident = gl16(p);
		q->perms = gl16(p);
		q->locks = gl16(p);
		gconv(p, conv, tmp, sizeof tmp);
		tmp[sizeof tmp - 1] = 0;
		q->path = estrdup9p(tmp);
		gconv(p, conv, tmp, sizeof tmp);
		tmp[sizeof tmp - 1] = 0;
		q->user = estrdup9p(tmp);
		q++;
	}
	free(p);
	if(ngot < navail)
		goto more;
	return ngot;
}
