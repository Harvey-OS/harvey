#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

static NamedInt	flagMap[] =
{
	{"\\Seen",	MSeen},
	{"\\Answered",	MAnswered},
	{"\\Flagged",	MFlagged},
	{"\\Deleted",	MDeleted},
	{"\\Draft",	MDraft},
	{"\\Recent",	MRecent},
	{nil,		0}
};

int
storeMsg(Box *box, Msg *m, int uids, void *vst)
{
	Store *st;
	int f, flags;

	USED(uids);

	if(m->expunged)
		return uids;

	st = vst;
	flags = st->flags;

	f = m->flags;
	if(st->sign == '+')
		f |= flags;
	else if(st->sign == '-')
		f &= ~flags;
	else
		f = flags;

	/*
	 * not allowed to change the recent flag
	 */
	f = (f & ~MRecent) | (m->flags & MRecent);
	setFlags(box, m, f);

	if(st->op != STFlagsSilent){
		m->sendFlags = 1;
		box->sendFlags = 1;
	}

	return 1;
}

/*
 * update flags & global flag counts in box
 */
void
setFlags(Box *box, Msg *m, int f)
{
	if(f == m->flags)
		return;

	box->dirtyImp = 1;
	if((f & MRecent) != (m->flags & MRecent)){
		if(f & MRecent)
			box->recent++;
		else
			box->recent--;
	}
	m->flags = f;
}

void
sendFlags(Box *box, int uids)
{
	Msg *m;

	if(!box->sendFlags)
		return;

	box->sendFlags = 0;
	for(m = box->msgs; m != nil; m = m->next){
		if(!m->expunged && m->sendFlags){
			Bprint(&bout, "* %lud FETCH (", m->seq);
			if(uids)
				Bprint(&bout, "uid %lud ", m->uid);
			Bprint(&bout, "FLAGS (");
			writeFlags(&bout, m, 1);
			Bprint(&bout, "))\r\n");
			m->sendFlags = 0;
		}
	}
}

void
writeFlags(Biobuf *b, Msg *m, int recentOk)
{
	char *sep;
	int f;

	sep = "";
	for(f = 0; flagMap[f].name != nil; f++){
		if((m->flags & flagMap[f].v)
		&& (flagMap[f].v != MRecent || recentOk)){
			Bprint(b, "%s%s", sep, flagMap[f].name);
			sep = " ";
		}
	}
}

int
msgSeen(Box *box, Msg *m)
{
	if(m->flags & MSeen)
		return 0;
	m->flags |= MSeen;
	box->sendFlags = 1;
	m->sendFlags = 1;
	box->dirtyImp = 1;
	return 1;
}

ulong
mapFlag(char *name)
{
	return mapInt(flagMap, name);
}
