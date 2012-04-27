#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

char *fetchPartNames[FPMax] =
{
	"",
	"HEADER",
	"HEADER.FIELDS",
	"HEADER.FIELDS.NOT",
	"MIME",
	"TEXT",
};

/*
 * implicitly set the \seen flag.  done in a separate pass
 * so the .imp file doesn't need to be open while the
 * messages are sent to the client.
 */
int
fetchSeen(Box *box, Msg *m, int uids, void *vf)
{
	Fetch *f;

	USED(uids);

	if(m->expunged)
		return uids;
	for(f = vf; f != nil; f = f->next){
		switch(f->op){
		case FRfc822:
		case FRfc822Text:
		case FBodySect:
			msgSeen(box, m);
			goto breakout;
		}
	}
breakout:

	return 1;
}

/*
 * fetch messages
 *
 * imap4 body[] requestes get translated to upas/fs files as follows
 *	body[id.header] == id/rawheader file + extra \r\n
 *	body[id.text] == id/rawbody
 *	body[id.mime] == id/mimeheader + extra \r\n
 *	body[id] === body[id.header] + body[id.text]
*/
int
fetchMsg(Box *, Msg *m, int uids, void *vf)
{
	Tm tm;
	Fetch *f;
	char *sep;
	int todo;

	if(m->expunged)
		return uids;

	todo = 0;
	for(f = vf; f != nil; f = f->next){
		switch(f->op){
		case FFlags:
			todo = 1;
			break;
		case FUid:
			todo = 1;
			break;
		case FInternalDate:
		case FEnvelope:
		case FRfc822:
		case FRfc822Head:
		case FRfc822Size:
		case FRfc822Text:
		case FBodySect:
		case FBodyPeek:
		case FBody:
		case FBodyStruct:
			todo = 1;
			if(!msgStruct(m, 1)){
				msgDead(m);
				return uids;
			}
			break;
		default:
			bye("bad implementation of fetch");
			return 0;
		}
	}

	if(m->expunged)
		return uids;
	if(!todo)
		return 1;

	/*
	 * note: it is allowed to send back the responses one at a time
	 * rather than all together.  this is exploited to send flags elsewhere.
	 */
	Bprint(&bout, "* %lud FETCH (", m->seq);
	sep = "";
	if(uids){
		Bprint(&bout, "UID %lud", m->uid);
		sep = " ";
	}
	for(f = vf; f != nil; f = f->next){
		switch(f->op){
		default:
			bye("bad implementation of fetch");
			break;
		case FFlags:
			Bprint(&bout, "%sFLAGS (", sep);
			writeFlags(&bout, m, 1);
			Bprint(&bout, ")");
			break;
		case FUid:
			if(uids)
				continue;
			Bprint(&bout, "%sUID %lud", sep, m->uid);
			break;
		case FEnvelope:
			Bprint(&bout, "%sENVELOPE ", sep);
			fetchEnvelope(m);
			break;
		case FInternalDate:
			Bprint(&bout, "%sINTERNALDATE ", sep);
			Bimapdate(&bout, date2tm(&tm, m->unixDate));
			break;
		case FBody:
			Bprint(&bout, "%sBODY ", sep);
			fetchBodyStruct(m, &m->head, 0);
			break;
		case FBodyStruct:
			Bprint(&bout, "%sBODYSTRUCTURE ", sep);
			fetchBodyStruct(m, &m->head, 1);
			break;
		case FRfc822Size:
			Bprint(&bout, "%sRFC822.SIZE %lud", sep, msgSize(m));
			break;
		case FRfc822:
			f->part = FPAll;
			Bprint(&bout, "%sRFC822", sep);
			fetchBody(m, f);
			break;
		case FRfc822Head:
			f->part = FPHead;
			Bprint(&bout, "%sRFC822.HEADER", sep);
			fetchBody(m, f);
			break;
		case FRfc822Text:
			f->part = FPText;
			Bprint(&bout, "%sRFC822.TEXT", sep);
			fetchBody(m, f);
			break;
		case FBodySect:
		case FBodyPeek:
			Bprint(&bout, "%sBODY", sep);
			fetchBody(fetchSect(m, f), f);
			break;
		}
		sep = " ";
	}
	Bprint(&bout, ")\r\n");

	return 1;
}

/*
 * print out section, part, headers;
 * find and return message section
 */
Msg *
fetchSect(Msg *m, Fetch *f)
{
	Bputc(&bout, '[');
	BNList(&bout, f->sect, ".");
	if(f->part != FPAll){
		if(f->sect != nil)
			Bputc(&bout, '.');
		Bprint(&bout, "%s", fetchPartNames[f->part]);
		if(f->hdrs != nil){
			Bprint(&bout, " (");
			BSList(&bout, f->hdrs, " ");
			Bputc(&bout, ')');
		}
	}
	Bprint(&bout, "]");
	return findMsgSect(m, f->sect);
}

/*
 * actually return the body pieces
 */
void
fetchBody(Msg *m, Fetch *f)
{
	Pair p;
	char *s, *t, *e, buf[BufSize + 2];
	ulong n, start, stop, pos;
	int fd, nn;

	if(m == nil){
		fetchBodyStr(f, "", 0);
		return;
	}
	switch(f->part){
	case FPHeadFields:
	case FPHeadFieldsNot:
		n = m->head.size + 3;
		s = emalloc(n);
		n = selectFields(s, n, m->head.buf, f->hdrs, f->part == FPHeadFields);
		fetchBodyStr(f, s, n);
		free(s);
		return;
	case FPHead:
		fetchBodyStr(f, m->head.buf, m->head.size);
		return;
	case FPMime:
		fetchBodyStr(f, m->mime.buf, m->mime.size);
		return;
	case FPAll:
		fd = msgFile(m, "rawbody");
		if(fd < 0){
			msgDead(m);
			fetchBodyStr(f, "", 0);
			return;
		}
		p = fetchBodyPart(f, msgSize(m));
		start = p.start;
		if(start < m->head.size){
			stop = p.stop;
			if(stop > m->head.size)
				stop = m->head.size;
			Bwrite(&bout, &m->head.buf[start], stop - start);
			start = 0;
			stop = p.stop;
			if(stop <= m->head.size){
				close(fd);
				return;
			}
		}else
			start -= m->head.size;
		stop = p.stop - m->head.size;
		break;
	case FPText:
		fd = msgFile(m, "rawbody");
		if(fd < 0){
			msgDead(m);
			fetchBodyStr(f, "", 0);
			return;
		}
		p = fetchBodyPart(f, m->size);
		start = p.start;
		stop = p.stop;
		break;
	default:
		fetchBodyStr(f, "", 0);
		return;
	}

	/*
	 * read in each block, convert \n without \r to \r\n.
	 * this means partial fetch requires fetching everything
	 * through stop, since we don't know how many \r's will be added
	 */
	buf[0] = ' ';
	for(pos = 0; pos < stop; ){
		n = BufSize;
		if(n > stop - pos)
			n = stop - pos;
		n = read(fd, &buf[1], n);
		if(n <= 0){
			fetchBodyFill(stop - pos);
			break;
		}
		e = &buf[n + 1];
		*e = '\0';
		for(s = &buf[1]; s < e && pos < stop; s = t + 1){
			t = memchr(s, '\n', e - s);
			if(t == nil)
				t = e;
			n = t - s;
			if(pos < start){
				if(pos + n <= start){
					s = t;
					pos += n;
				}else{
					s += start - pos;
					pos = start;
				}
				n = t - s;
			}
			nn = n;
			if(pos + nn > stop)
				nn = stop - pos;
			if(Bwrite(&bout, s, nn) != nn)
				writeErr();
			pos += n;
			if(*t == '\n'){
				if(t[-1] != '\r'){
					if(pos >= start && pos < stop)
						Bputc(&bout, '\r');
					pos++;
				}
				if(pos >= start && pos < stop)
					Bputc(&bout, '\n');
				pos++;
			}
		}
		buf[0] = e[-1];
	}
	close(fd);
}

/*
 * resolve the actual bounds of any partial fetch,
 * and print out the bounds & size of string returned
 */
Pair
fetchBodyPart(Fetch *f, ulong size)
{
	Pair p;
	ulong start, stop;

	start = 0;
	stop = size;
	if(f->partial){
		start = f->start;
		if(start > size)
			start = size;
		stop = start + f->size;
		if(stop > size)
			stop = size;
		Bprint(&bout, "<%lud>", start);
	}
	Bprint(&bout, " {%lud}\r\n", stop - start);
	p.start = start;
	p.stop = stop;
	return p;
}

/*
 * something went wrong fetching data
 * produce fill bytes for what we've committed to produce
 */
void
fetchBodyFill(ulong n)
{
	while(n-- > 0)
		if(Bputc(&bout, ' ') < 0)
			writeErr();
}

/*
 * return a simple string
 */
void
fetchBodyStr(Fetch *f, char *buf, ulong size)
{
	Pair p;

	p = fetchBodyPart(f, size);
	Bwrite(&bout, &buf[p.start], p.stop-p.start);
}

char*
printnlist(NList *sect)
{
	static char buf[100];
	char *p;

	for(p= buf; sect; sect=sect->next){
		p += sprint(p, "%ld", sect->n);
		if(sect->next)
			*p++ = '.';
	}
	*p = '\0';
	return buf;
}

/*
 * find the numbered sub-part of the message
 */
Msg*
findMsgSect(Msg *m, NList *sect)
{
	ulong id;

	for(; sect != nil; sect = sect->next){
		id = sect->n;
#ifdef HACK
		/* HACK to solve extra level of structure not visible from upas/fs  */
		if(m->kids == 0 && id == 1 && sect->next == nil){
			if(m->mime.type->s && strcmp(m->mime.type->s, "message")==0)
			if(m->mime.type->t && strcmp(m->mime.type->t, "rfc822")==0)
			if(m->head.type->s && strcmp(m->head.type->s, "text")==0)
			if(m->head.type->t && strcmp(m->head.type->t, "plain")==0)
				break;
		}
		/* end of HACK */
#endif HACK
		for(m = m->kids; m != nil; m = m->next)
			if(m->id == id)
				break;
		if(m == nil)
			return nil;
	}
	return m;
}

void
fetchEnvelope(Msg *m)
{
	Tm tm;

	Bputc(&bout, '(');
	Brfc822date(&bout, date2tm(&tm, m->info[IDate]));
	Bputc(&bout, ' ');
	Bimapstr(&bout, m->info[ISubject]);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->from);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->sender);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->replyTo);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->to);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->cc);
	Bputc(&bout, ' ');
	Bimapaddr(&bout, m->bcc);
	Bputc(&bout, ' ');
	Bimapstr(&bout, m->info[IInReplyTo]);
	Bputc(&bout, ' ');
	Bimapstr(&bout, m->info[IMessageId]);
	Bputc(&bout, ')');
}

void
fetchBodyStruct(Msg *m, Header *h, int extensions)
{
	Msg *k;
	ulong len;

	if(msgIsMulti(h)){
		Bputc(&bout, '(');
		for(k = m->kids; k != nil; k = k->next)
			fetchBodyStruct(k, &k->mime, extensions);

		Bputc(&bout, ' ');
		Bimapstr(&bout, h->type->t);

		if(extensions){
			Bputc(&bout, ' ');
			BimapMimeParams(&bout, h->type->next);
			fetchStructExt(h);
		}

		Bputc(&bout, ')');
		return;
	}

	Bputc(&bout, '(');
	if(h->type != nil){
		Bimapstr(&bout, h->type->s);
		Bputc(&bout, ' ');
		Bimapstr(&bout, h->type->t);
		Bputc(&bout, ' ');
		BimapMimeParams(&bout, h->type->next);
	}else
		Bprint(&bout, "\"text\" \"plain\" NIL");

	Bputc(&bout, ' ');
	if(h->id != nil)
		Bimapstr(&bout, h->id->s);
	else
		Bprint(&bout, "NIL");

	Bputc(&bout, ' ');
	if(h->description != nil)
		Bimapstr(&bout, h->description->s);
	else
		Bprint(&bout, "NIL");

	Bputc(&bout, ' ');
	if(h->encoding != nil)
		Bimapstr(&bout, h->encoding->s);
	else
		Bprint(&bout, "NIL");

	/*
	 * this is so strange: return lengths for a body[text] response,
	 * except in the case of a multipart message, when return lengths for a body[] response
	 */
	len = m->size;
	if(h == &m->mime)
		len += m->head.size;
	Bprint(&bout, " %lud", len);

	len = m->lines;
	if(h == &m->mime)
		len += m->head.lines;

	if(h->type == nil || cistrcmp(h->type->s, "text") == 0){
		Bprint(&bout, " %lud", len);
	}else if(msgIsRfc822(h)){
		Bputc(&bout, ' ');
		k = m;
		if(h != &m->mime)
			k = m->kids;
		if(k == nil)
			Bprint(&bout, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (\"text\" \"plain\" NIL NIL NIL NIL 0 0) 0");
		else{
			fetchEnvelope(k);
			Bputc(&bout, ' ');
			fetchBodyStruct(k, &k->head, extensions);
			Bprint(&bout, " %lud", len);
		}
	}

	if(extensions){
		Bputc(&bout, ' ');

		/*
		 * don't have the md5 laying around,
		 * since the header & body have added newlines.
		 */
		Bprint(&bout, "NIL");

		fetchStructExt(h);
	}
	Bputc(&bout, ')');
}

/*
 * common part of bodystructure extensions
 */
void
fetchStructExt(Header *h)
{
	Bputc(&bout, ' ');
	if(h->disposition != nil){
		Bputc(&bout, '(');
		Bimapstr(&bout, h->disposition->s);
		Bputc(&bout, ' ');
		BimapMimeParams(&bout, h->disposition->next);
		Bputc(&bout, ')');
	}else
		Bprint(&bout, "NIL");
	Bputc(&bout, ' ');
	if(h->language != nil){
		if(h->language->next != nil)
			BimapMimeParams(&bout, h->language->next);
		else
			Bimapstr(&bout, h->language->s);
	}else
		Bprint(&bout, "NIL");
}

int
BimapMimeParams(Biobuf *b, MimeHdr *mh)
{
	char *sep;
	int n;

	if(mh == nil)
		return Bprint(b, "NIL");

	n = Bputc(b, '(');

	sep = "";
	for(; mh != nil; mh = mh->next){
		n += Bprint(b, sep);
		n += Bimapstr(b, mh->s);
		n += Bputc(b, ' ');
		n += Bimapstr(b, mh->t);
		sep = " ";
	}

	n += Bputc(b, ')');
	return n;
}

/*
 * print a list of addresses;
 * each address is printed as '(' personalName AtDomainList mboxName hostName ')'
 * the AtDomainList is always NIL
 */
int
Bimapaddr(Biobuf *b, MAddr *a)
{
	char *host, *sep;
	int n;

	if(a == nil)
		return Bprint(b, "NIL");

	n = Bputc(b, '(');
	sep = "";
	for(; a != nil; a = a->next){
		n += Bprint(b, "%s(", sep);
		n += Bimapstr(b, a->personal);
		n += Bprint(b," NIL ");
		n += Bimapstr(b, a->box);
		n += Bputc(b, ' ');

		/*
		 * can't send NIL as hostName, since that is code for a group
		 */
		host = a->host;
		if(host == nil)
			host = "";
		n += Bimapstr(b, host);

		n += Bputc(b, ')');
		sep = " ";
	}
	n += Bputc(b, ')');
	return n;
}
