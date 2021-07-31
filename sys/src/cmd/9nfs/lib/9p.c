#include "all.h"

static char *tnames[] = {
	[Tnop]		"nop",
	[Tflush]	"flush",
	[Tclone]	"clone",
	[Twalk]		"walk",
	[Topen]		"open",
	[Tcreate]	"create",
	[Tread]		"read",
	[Twrite]	"write",
	[Tclunk]	"clunk",
	[Tremove]	"remove",
	[Tstat]		"stat",
	[Twstat]	"wstat",
	[Tclwalk]	"clwalk",
	[Tsession]	"session",
	[Tattach]	"attach",
};

int
xmesg(Session *s, int t)
{
	int n;

	if(chatty){
		if(0 <= t && t < nelem(tnames) && tnames[t])
			chat("T%s...", tnames[t]);
		else
			chat("T%d...", t);
	}
	s->f.type = t;
	s->f.tag = ++s->tag;
	n = convS2M(&s->f, s->data);
	if(niwrite(s->fd, s->data, n) != n){
		clog("xmesg write error on %d: %r\n", s->fd);
		return -1;
	}
again:
	n = niread(s->fd, s->data, sizeof s->data);
	if(n < 0){
		clog("xmesg read error: %r\n");
		return -1;
	}
	if(convM2S(s->data, &s->f, n) <= 0){
		if(n == 2 && s->data[0] == 'O' && s->data[1] == 'K')
			goto again;
		clog("xmesg bad convM2S %d %.2x %.2x %.2x %.2x\n",
			n, ((uchar*)s->data)[0], ((uchar*)s->data)[1],
			((uchar*)s->data)[2], ((uchar*)s->data)[3]);
		return -1;
	}
	if(s->f.tag != s->tag){
		clog("xmesg tag %d for %d\n", s->f.tag, s->tag);
		goto again;
	}
	if(s->f.type == Rerror){
		if(t == Tclunk)
			clog("xmesg clunk: %s", s->f.ename);
		chat("%s...", s->f.ename);
		return -1;
	}
	if(s->f.type != t+1){
		clog("xmesg type mismatch: %d, expected %d\n", s->f.type, t+1);
		return -1;
	}
	return 0;
}

int
clunkfid(Session *s, Fid *f)
{
	putfid(s, f);
	if(s == 0 || f == 0)
		return 0;
	s->f.fid = f - s->fids;
	return xmesg(s, Tclunk);
}

#define	UNLINK(p)	((p)->prev->next = (p)->next, (p)->next->prev = (p)->prev)

#define	LINK(h, p)	((p)->next = (h)->next, (p)->prev = (h), \
			 (h)->next->prev = (p), (h)->next = (p))

#define	TOFRONT(h, p)	((h)->next != (p) ? (UNLINK(p), LINK(h,p)) : 0)

Fid *
newfid(Session *s)
{
	Fid *f, *fN;

	chat("newfid..");
	if(s->list.prev == 0){
		chat("init..");
		s->list.prev = &s->list;
		s->list.next = &s->list;
		s->free = s->fids;
		if(chatty)
			fN = &s->fids[25];
		else
			fN = &s->fids[nelem(s->fids)];
		for(f=s->fids; f<fN; f++){
			f->owner = 0;
			f->prev = 0;
			f->next = f+1;
		}
		(f-1)->next = 0;
	}
	if(s->free){
		f = s->free;
		s->free = f->next;
		LINK(&s->list, f);
	}else{
		for(f=s->list.prev; f!=&s->list; f=f->prev)
			if(f->owner)
				break;
		if(f == &s->list){
			clog("fid leak");
			return 0;
		}
		setfid(s, f);
		if(xmesg(s, Tclunk) < 0){
			clog("clunk failed, no fids?");
			/*return 0;*/
		}
		*(f->owner) = 0;
		f->owner = 0;
	}
	chat("%d...", f - s->fids);
	f->tstale = nfstime + staletime;
	return f;
}

void
setfid(Session *s, Fid *f)
{
	TOFRONT(&s->list, f);
	f->tstale = nfstime + staletime;
	s->f.fid = f - s->fids;
}

void
putfid(Session *s, Fid *f)
{
	chat("putfid %d...", f-s->fids);
	if(s == 0 || f == 0){
		clog("putfid(0x%x, 0x%x) %s", s, f, (s ? s->service : "?"));
		return;
	}
	UNLINK(f);
	if(f->owner)
		*(f->owner) = 0;
	f->owner = 0;
	f->prev = 0;
	f->next = s->free;
	s->free = f;
}

void
fidtimer(Session *s, long now)
{
	Fid *f, *t;
	int n;

	f = s->list.next;
	n = 0;
	while(f != &s->list){
		t = f;
		f = f->next;
		if(t->owner && now >= t->tstale)
			++n, clunkfid(s, t);
	}
	if(n > 0)
		chat("fidtimer %s\n", s->service);
}
