#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

static int	dateCmp(char *date, Search *s);
static int	addrSearch(MAddr *a, char *s);
static int	fileSearch(Msg *m, char *file, char *pat);
static int	headerSearch(Msg *m, char *hdr, char *pat);

/*
 * free to exit, parseErr, since called before starting any client reply
 *
 * the header and envelope searches should convert mime character set escapes.
 */
int
searchMsg(Msg *m, Search *s)
{
	MsgSet *ms;
	int ok;

	if(!msgStruct(m, 1) || m->expunged)
		return 0;
	for(ok = 1; ok && s != nil; s = s->next){
		switch(s->key){
		default:
			ok = 0;
			break;
		case SKNot:
			ok = !searchMsg(m, s->left);
			break;
		case SKOr:
			ok = searchMsg(m, s->left) || searchMsg(m, s->right);
			break;
		case SKAll:
			ok = 1;
			break;
		case SKAnswered:
			ok = (m->flags & MAnswered) == MAnswered;
			break;
		case SKDeleted:
			ok = (m->flags & MDeleted) == MDeleted;
			break;
		case SKDraft:
			ok = (m->flags & MDraft) == MDraft;
			break;
		case SKFlagged:
			ok = (m->flags & MFlagged) == MFlagged;
			break;
		case SKKeyword:
			ok = (m->flags & s->num) == s->num;
			break;
		case SKNew:
			ok = (m->flags & (MRecent|MSeen)) == MRecent;
			break;
		case SKOld:
			ok = (m->flags & MRecent) != MRecent;
			break;
		case SKRecent:
			ok = (m->flags & MRecent) == MRecent;
			break;
		case SKSeen:
			ok = (m->flags & MSeen) == MSeen;
			break;
		case SKUnanswered:
			ok = (m->flags & MAnswered) != MAnswered;
			break;
		case SKUndeleted:
			ok = (m->flags & MDeleted) != MDeleted;
			break;
		case SKUndraft:
			ok = (m->flags & MDraft) != MDraft;
			break;
		case SKUnflagged:
			ok = (m->flags & MFlagged) != MFlagged;
			break;
		case SKUnkeyword:
			ok = (m->flags & s->num) != s->num;
			break;
		case SKUnseen:
			ok = (m->flags & MSeen) != MSeen;
			break;

		case SKLarger:
			ok = msgSize(m) > s->num;
			break;
		case SKSmaller:
			ok = msgSize(m) < s->num;
			break;

		case SKBcc:
			ok = addrSearch(m->bcc, s->s);
			break;
		case SKCc:
			ok = addrSearch(m->cc, s->s);
			break;
		case SKFrom:
			ok = addrSearch(m->from, s->s);
			break;
		case SKTo:
			ok = addrSearch(m->to, s->s);
			break;
		case SKSubject:
			ok = 0;
			if(m->info[ISubject])
				ok = cistrstr(m->info[ISubject], s->s) != nil;
			break;

		case SKBefore:
			ok = dateCmp(m->unixDate, s) < 0;
			break;
		case SKOn:
			ok = dateCmp(m->unixDate, s) == 0;
			break;
		case SKSince:
			ok = dateCmp(m->unixDate, s) > 0;
			break;
		case SKSentBefore:
			ok = dateCmp(m->info[IDate], s) < 0;
			break;
		case SKSentOn:
			ok = dateCmp(m->info[IDate], s) == 0;
			break;
		case SKSentSince:
			ok = dateCmp(m->info[IDate], s) > 0;
			break;

		case SKUid:
		case SKSet:
			for(ms = s->set; ms != nil; ms = ms->next)
				if(s->key == SKUid && m->uid >= ms->from && m->uid <= ms->to
				|| s->key == SKSet && m->seq >= ms->from && m->seq <= ms->to)
					break;
			ok = ms != nil;
			break;

		case SKHeader:
			ok = headerSearch(m, s->hdr, s->s);
			break;

		case SKBody:
		case SKText:
			if(s->key == SKText && cistrstr(m->head.buf, s->s)){
				ok = 1;
				break;
			}
			ok = fileSearch(m, "body", s->s);
			break;
		}
	}
	return ok;
}

static int
fileSearch(Msg *m, char *file, char *pat)
{
	char buf[BufSize + 1];
	int n, nbuf, npat, fd, ok;

	npat = strlen(pat);
	if(npat >= BufSize / 2)
		return 0;

	fd = msgFile(m, file);
	if(fd < 0)
		return 0;
	ok = 0;
	nbuf = 0;
	for(;;){
		n = read(fd, &buf[nbuf], BufSize - nbuf);
		if(n <= 0)
			break;
		nbuf += n;
		buf[nbuf] = '\0';
		if(cistrstr(buf, pat) != nil){
			ok = 1;
			break;
		}
		if(nbuf > npat){
			memmove(buf, &buf[nbuf - npat], npat);
			nbuf = npat;
		}
	}
	close(fd);
	return ok;
}

static int
headerSearch(Msg *m, char *hdr, char *pat)
{
	SList hdrs;
	char *s, *t;
	int ok, n;

	n = m->head.size + 3;
	s = emalloc(n);
	hdrs.next = nil;
	hdrs.s = hdr;
	ok = 0;
	if(selectFields(s, n, m->head.buf, &hdrs, 1) > 0){
		t = strchr(s, ':');
		if(t != nil && cistrstr(t+1, pat) != nil)
			ok = 1;
	}
	free(s);
	return ok;
}

static int
addrSearch(MAddr *a, char *s)
{
	char *ok, *addr;

	for(; a != nil; a = a->next){
		addr = maddrStr(a);
		ok = cistrstr(addr, s);
		free(addr);
		if(ok != nil)
			return 1;
	}
	return 0;
}

static int
dateCmp(char *date, Search *s)
{
	Tm tm;

	date2tm(&tm, date);
	if(tm.year < s->year)
		return -1;
	if(tm.year > s->year)
		return 1;
	if(tm.mon < s->mon)
		return -1;
	if(tm.mon > s->mon)
		return 1;
	if(tm.mday < s->mday)
		return -1;
	if(tm.mday > s->mday)
		return 1;
	return 0;
}
