#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>
#include <auth.h>
#include <fcall.h>
#include "imap4d.h"

static void	body64(int in, int out);
static void	bodystrip(int in, int out);
static void	cleanupHeader(Header *h);
static char	*domBang(char *s);
static void	freeMAddr(MAddr *a);
static void	freeMimeHdr(MimeHdr *mh);
static char	*headAddrSpec(char *e, char *w);
static MAddr	*headAddresses(void);
static MAddr	*headAddress(void);
static char	*headAtom(char *disallowed);
static int	headChar(int eat);
static char	*headDomain(char *e);
static MAddr	*headMAddr(MAddr *old);
static char	*headPhrase(char *e, char *w);
static char	*headQuoted(int start, int stop);
static char	*headSkipWhite(int);
static void	headSkip(void);
static char	*headSubDomain(void);
static char	*headText(void);
static void	headToEnd(void);
static char	*headWord(void);
static void	mimeDescription(Header *h);
static void	mimeDisposition(Header *h);
static void	mimeEncoding(Header *h);
static void	mimeId(Header *h);
static void	mimeLanguage(Header *h);
static void	mimeMd5(Header *h);
static MimeHdr	*mimeParams(void);
static void	mimeType(Header *h);
static MimeHdr	*mkMimeHdr(char *s, char *t, MimeHdr *next);
static void	msgAddDate(Msg *m);
static void	msgAddHead(Msg *m, char *head, char *body);
static int	msgBodySize(Msg *m);
static int	msgHeader(Msg *m, Header *h, char *file);
static long	msgReadFile(Msg *m, char *file, char **ss);
static int	msgUnix(Msg *m, int top);
static void	stripQuotes(char *q);
static MAddr	*unixFrom(char *s);


static char bogusBody[] = 
	"This message contains null characters, so it cannot be displayed correctly.\r\n"
	"Most likely you were sent a bogus message or a binary file.\r\n"
	"\r\n"
	"Each of the following attachments has a different version of the message.\r\n"
	"The first is inlined with all non-printable characters stripped.\r\n"
	"The second contains the message as it was stored in your mailbox.\r\n"
	"The third has the initial header stripped.\r\n";

static char bogusMimeText[] =
	"Content-Disposition: inline\r\n"
	"Content-Type: text/plain; charset=\"US-ASCII\"\r\n"
	"Content-Transfer-Encoding: 7bit\r\n";

static char bogusMimeBinary[] =
	"Content-Disposition: attachment\r\n"
	"Content-Type: application/octet-stream\r\n"
	"Content-Transfer-Encoding: base64\r\n";

/*
 * stop list for header fields
 */
static char	*headFieldStop = ":";
static char	*mimeTokenStop = "()<>@,;:\\\"/[]?=";
static char	*headAtomStop = "()<>@,;:\\\".[]";
static uchar	*headStr;
static uchar	*lastWhite;

long
selectFields(char *dst, long n, char *hdr, SList *fields, int matches)
{
	SList *f;
	uchar *start;
	char *s;
	long m, nf;

	headStr = (uchar*)hdr;
	m = 0;
	for(;;){
		start = headStr;
		s = headAtom(headFieldStop);
		if(s == nil)
			break;
		headSkip();
		for(f = fields; f != nil; f = f->next){
			if(cistrcmp(s, f->s) == !matches){
				nf = headStr - start;
				if(m + nf > n)
					return 0;
				memmove(&dst[m], start, nf);
				m += nf;
			}
		}
		free(s);
	}
	if(m + 3 > n)
		return 0;
	dst[m++] = '\r';
	dst[m++] = '\n';
	dst[m] = '\0';
	return m;
}

void
freeMsg(Msg *m)
{
	Msg *k, *last;

	free(m->iBuf);
	freeMAddr(m->to);
	if(m->replyTo != m->from)
		freeMAddr(m->replyTo);
	if(m->sender != m->from)
		freeMAddr(m->sender);
	if(m->from != m->unixFrom)
		freeMAddr(m->from);
	freeMAddr(m->unixFrom);
	freeMAddr(m->cc);
	freeMAddr(m->bcc);
	free(m->unixDate);
	cleanupHeader(&m->head);
	cleanupHeader(&m->mime);
	for(k = m->kids; k != nil; ){
		last = k;
		k = k->next;
		freeMsg(last);
	}
	free(m->fs);
	free(m);
}

ulong
msgSize(Msg *m)
{
	return m->head.size + m->size;
}

int
infoIsNil(char *s)
{
	return s == nil || s[0] == '\0';
}

char*
maddrStr(MAddr *a)
{
	char *host, *addr;
	int n;

	host = a->host;
	if(host == nil)
		host = "";
	n = strlen(a->box) + strlen(host) + 2;
	if(a->personal != nil)
		n += strlen(a->personal) + 3;
	addr = emalloc(n);
	if(a->personal != nil)
		snprint(addr, n, "%s <%s@%s>", a->personal, a->box, host);
	else
		snprint(addr, n, "%s@%s", a->box, host);
	return addr;
}

/*
 * return actual name of f in m's fs directory
 * this is special cased when opening m/rawbody, m/mimeheader, or m/rawheader,
 * if the message was corrupted.  in that case,
 * a temporary file is made to hold the base64 encoding of m/raw.
 */
int
msgFile(Msg *m, char *f)
{
	Msg *parent, *p;
	Dir d;
	Tm tm;
	char buf[64], nbuf[2];
	uchar dbuf[64];
	int i, n, fd, fd1, fd2;

	if(!m->bogus
	|| strcmp(f, "") != 0 && strcmp(f, "rawbody") != 0
	&& strcmp(f, "rawheader") != 0 && strcmp(f, "mimeheader") != 0
	&& strcmp(f, "info") != 0 && strcmp(f, "unixheader") != 0){
		if(strlen(f) > MsgNameLen)
			bye("internal error: msgFile name too long");
		strcpy(m->efs, f);
		return cdOpen(m->fsDir, m->fs, OREAD);
	}

	/*
	 * walk up the stupid runt message parts for non-multipart messages
	 */
	parent = m->parent;
	if(parent != nil && parent->parent != nil){
		m = parent;
		parent = m->parent;
	}
	p = m;
	if(parent != nil)
		p = parent;

	if(strcmp(f, "info") == 0 || strcmp(f, "unixheader") == 0){
		strcpy(p->efs, f);
		return cdOpen(p->fsDir, p->fs, OREAD);
	}

	fd = imapTmp();
	if(fd < 0)
		return -1;

	/*
	 * craft the message parts for bogus messages
	 */
	if(strcmp(f, "") == 0){
		/*
		 * make a fake directory for each kid
		 * all we care about is the name
		 */
		if(parent == nil){
			nulldir(&d);
			d.mode = DMDIR|0600;
			d.qid.type = QTDIR;
			d.name = nbuf;
			nbuf[1] = '\0';
			for(i = '1'; i <= '4'; i++){
				nbuf[0] = i;
				n = convD2M(&d, dbuf, sizeof(dbuf));
				if(n <= BIT16SZ)
					fprint(2, "bad convD2M %d\n", n);
				write(fd, dbuf, n);
			}
		}
	}else if(strcmp(f, "mimeheader") == 0){
		if(parent != nil){
			switch(m->id){
			case 1:
			case 2:
				fprint(fd, "%s", bogusMimeText);
				break;
			case 3:
			case 4:
				fprint(fd, "%s", bogusMimeBinary);
				break;
			}
		}
	}else if(strcmp(f, "rawheader") == 0){
		if(parent == nil){
			date2tm(&tm, m->unixDate);
			rfc822date(buf, sizeof(buf), &tm);
			fprint(fd,
				"Date: %s\r\n"
				"From: imap4 daemon <%s@%s>\r\n"
				"To: <%s@%s>\r\n"
				"Subject: This message was illegal or corrupted\r\n"
				"MIME-Version: 1.0\r\n"
				"Content-Type: multipart/mixed;\r\n\tboundary=\"upas-%s\"\r\n",
					buf, username, site, username, site, m->info[IDigest]);
		}
	}else if(strcmp(f, "rawbody") == 0){
		fd1 = msgFile(p, "raw");
		strcpy(p->efs, "rawbody");
		fd2 = cdOpen(p->fsDir, p->fs, OREAD);
		if(fd1 < 0 || fd2 < 0){
			close(fd);
			close(fd1);
			close(fd2);
			return -1;
		}
		if(parent == nil){
			fprint(fd,
				"This is a multi-part message in MIME format.\r\n"
				"--upas-%s\r\n"
				"%s"
				"\r\n"
				"%s"
				"\r\n",
					m->info[IDigest], bogusMimeText, bogusBody);

			fprint(fd,
				"--upas-%s\r\n"
				"%s"
				"\r\n",
					m->info[IDigest], bogusMimeText);
			bodystrip(fd1, fd);

			fprint(fd,
				"--upas-%s\r\n"
				"%s"
				"\r\n",
					m->info[IDigest], bogusMimeBinary);
			seek(fd1, 0, 0);
			body64(fd1, fd);

			fprint(fd,
				"--upas-%s\r\n"
				"%s"
				"\r\n",
					m->info[IDigest], bogusMimeBinary);
			body64(fd2, fd);

			fprint(fd, "--upas-%s--\r\n", m->info[IDigest]);
		}else{
			switch(m->id){
			case 1:
				fprint(fd, "%s", bogusBody);
				break;
			case 2:
				bodystrip(fd1, fd);
				break;
			case 3:
				body64(fd1, fd);
				break;
			case 4:
				body64(fd2, fd);
				break;
			}
		}
		close(fd1);
		close(fd2);
	}
	seek(fd, 0, 0);
	return fd;
}

int
msgIsMulti(Header *h)
{
	return h->type != nil && cistrcmp("multipart", h->type->s) == 0;
}

int
msgIsRfc822(Header *h)
{
	return h->type != nil && cistrcmp("message", h->type->s) == 0 && cistrcmp("rfc822", h->type->t) == 0;
}

/*
 * check if a message has been deleted by someone else
 */
void
msgDead(Msg *m)
{
	if(m->expunged)
		return;
	*m->efs = '\0';
	if(!cdExists(m->fsDir, m->fs))
		m->expunged = 1;
}

/*
 * make sure the message has valid associated info
 * used for ISubject, IDigest, IInReplyTo, IMessageId.
 */
int
msgInfo(Msg *m)
{
	char *s;
	int i;

	if(m->info[0] != nil)
		return 1;

	i = msgReadFile(m, "info", &m->iBuf);
	if(i < 0)
		return 0;

	s = m->iBuf;
	for(i = 0; i < IMax; i++){
		m->info[i] = s;
		s = strchr(s, '\n');
		if(s == nil)
			break;
		*s++ = '\0';
	}
	for(; i < IMax; i++)
		m->info[i] = nil;

	for(i = 0; i < IMax; i++)
		if(infoIsNil(m->info[i]))
			m->info[i] = nil;

	return 1;
}

/*
 * make sure the message has valid mime structure
 * and sub-messages
 */
int
msgStruct(Msg *m, int top)
{
	Msg *k, head, *last;
	Dir *d;
	char *s;
	ulong max, id;
	int i, nd, fd, ns;

	if(m->kids != nil)
		return 1;

	if(m->expunged
	|| !msgInfo(m)
	|| !msgUnix(m, top)
	|| !msgBodySize(m)
	|| !msgHeader(m, &m->mime, "mimeheader")
	|| (top || msgIsRfc822(&m->mime) || msgIsMulti(&m->mime)) && !msgHeader(m, &m->head, "rawheader")){
		if(top && m->bogus && !(m->bogus & BogusTried)){
			m->bogus |= BogusTried;
			return msgStruct(m, top);
		}
		msgDead(m);
		return 0;
	}

	/*
	 * if a message has no kids, it has a kid which is just the body of the real message
	 */
	if(!msgIsMulti(&m->head) && !msgIsMulti(&m->mime) && !msgIsRfc822(&m->head) && !msgIsRfc822(&m->mime)){
		k = MKZ(Msg);
		k->id = 1;
		k->fsDir = m->fsDir;
		k->bogus = m->bogus;
		k->parent = m->parent;
		ns = m->efs - m->fs;
		k->fs = emalloc(ns + (MsgNameLen + 1));
		memmove(k->fs, m->fs, ns);
		k->efs = k->fs + ns;
		*k->efs = '\0';
		k->size = m->size;
		m->kids = k;
		return 1;
	}

	/*
	 * read in all child messages messages
	 */
	fd = msgFile(m, "");
	if(fd < 0){
		msgDead(m);
		return 0;
	}

	max = 0;
	head.next = nil;
	last = &head;
	while((nd = dirread(fd, &d)) > 0){
		for(i = 0; i < nd; i++){
			s = d[i].name;
			id = strtol(s, &s, 10);
			if(id <= max || *s != '\0'
			|| (d[i].mode & DMDIR) != DMDIR)
				continue;

			max = id;

			k = MKZ(Msg);
			k->id = id;
			k->fsDir = m->fsDir;
			k->bogus = m->bogus;
			k->parent = m;
			ns = strlen(m->fs);
			k->fs = emalloc(ns + 2 * (MsgNameLen + 1));
			k->efs = seprint(k->fs, k->fs + ns + (MsgNameLen + 1), "%s%lud/", m->fs, id);
			k->prev = last;
			k->size = ~0UL;
			k->lines = ~0UL;
			last->next = k;
			last = k;
		}
	}
	close(fd);
	m->kids = head.next;

	/*
	 * if kids fail, just whack them
	 */
	top = top && (msgIsRfc822(&m->head) || msgIsMulti(&m->head));
	for(k = m->kids; k != nil; k = k->next){
		if(!msgStruct(k, top)){
			for(k = m->kids; k != nil; ){
				last = k;
				k = k->next;
				freeMsg(last);
			}
			m->kids = nil;
			break;
		}
	}
	return 1;
}

static long
msgReadFile(Msg *m, char *file, char **ss)
{
	Dir *d;
	char *s, buf[BufSize];
	vlong length;
	long n, nn;
	int fd;

	fd = msgFile(m, file);
	if(fd < 0){
		msgDead(m);
		return -1;
	}

	n = read(fd, buf, BufSize);
	if(n < BufSize){
		close(fd);
		if(n < 0){
			*ss = nil;
			return -1;
		}
		s = emalloc(n + 1);
		memmove(s, buf, n);
		s[n] = '\0';
		*ss = s;
		return n;
	}

	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return -1;
	}
	length = d->length;
	free(d);
	nn = length;
	s = emalloc(nn + 1);
	memmove(s, buf, n);
	if(nn > n)
		nn = readn(fd, s+n, nn-n) + n;
	close(fd);
	if(nn != length){
		free(s);
		return -1;
	}
	s[nn] = '\0';
	*ss = s;
	return nn;
}

static void
freeMAddr(MAddr *a)
{
	MAddr *p;

	while(a != nil){
		p = a;
		a = a->next;
		free(p->personal);
		free(p->box);
		free(p->host);
		free(p);
	}
}

/*
 * the message is corrupted or illegal.
 * reset message fields.  msgStruct will reparse the message,
 * relying on msgFile to make up corrected body parts.
 */
static int
msgBogus(Msg *m, int flags)
{
	if(!(m->bogus & flags))
		m->bogus |= flags;
	m->lines = ~0;
	free(m->head.buf);
	free(m->mime.buf);
	memset(&m->head, 0, sizeof(Header));
	memset(&m->mime, 0, sizeof(Header));
	return 0;
}

/*
 *  stolen from upas/marshal; base64 encodes from one fd to another.
 *
 *  the size of buf is very important to enc64.  Anything other than
 *  a multiple of 3 will cause enc64 to output a termination sequence.
 *  To ensure that a full buf corresponds to a multiple of complete lines,
 *  we make buf a multiple of 3*18 since that's how many enc64 sticks on
 *  a single line.  This avoids short lines in the output which is pleasing
 *  but not necessary.
 */
static int
enc64x18(char *out, int lim, uchar *in, int n)
{
	int m, mm, nn;

	nn = 0;
	for(; n > 0; n -= m){
		m = 18 * 3;
		if(m > n)
			m = n;
		mm = enc64(out, lim - nn, in, m);
		in += m;
		out += mm;
		*out++ = '\r';
		*out++ = '\n';
		nn += mm + 2;
	}
	return nn;
}

static void
body64(int in, int out)
{
	uchar buf[3*18*54];
	char obuf[3*18*54*2];
	int m, n;

	for(;;){
		n = read(in, buf, sizeof(buf));
		if(n < 0)
			return;
		if(n == 0)
			break;
		m = enc64x18(obuf, sizeof(obuf), buf, n);
		if(write(out, obuf, m) < 0)
			return;
	}
}

/*
 * strip all non-printable characters from a file
 */
static void
bodystrip(int in, int out)
{
	uchar buf[3*18*54];
	int m, n, i, c;

	for(;;){
		n = read(in, buf, sizeof(buf));
		if(n < 0)
			return;
		if(n == 0)
			break;
		m = 0;
		for(i = 0; i < n; i++){
			c = buf[i];
			if(c > 0x1f && c < 0x7f		/* normal characters */
			|| c >= 0x9 && c <= 0xd)	/* \t, \n, vertical tab, form feed, \r */
				buf[m++] = c;
		}

		if(m && write(out, buf, m) < 0)
			return;
	}
}

/*
 * read in the message body to count \n without a preceding \r
 */
static int
msgBodySize(Msg *m)
{
	Dir *d;
	char buf[BufSize + 2], *s, *se;
	vlong length;
	ulong size, lines, bad;
	int n, fd, c;

	if(m->lines != ~0UL)
		return 1;
	fd = msgFile(m, "rawbody");
	if(fd < 0)
		return 0;
	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return 0;
	}
	length = d->length;
	free(d);

	size = 0;
	lines = 0;
	bad = 0;
	buf[0] = ' ';
	for(;;){
		n = read(fd, &buf[1], BufSize);
		if(n <= 0)
			break;
		size += n;
		se = &buf[n + 1];
		for(s = &buf[1]; s < se; s++){
			c = *s;
			if(c == '\0'){
				close(fd);
				return msgBogus(m, BogusBody);
			}
			if(c != '\n')
				continue;
			if(s[-1] != '\r')
				bad++;
			lines++;
		}
		buf[0] = buf[n];
	}
	if(size != length)
		bye("bad length reading rawbody");
	size += bad;
	m->size = size;
	m->lines = lines;
	close(fd);
	return 1;
}

/*
 * retrieve information from the unixheader file
 */
static int
msgUnix(Msg *m, int top)
{
	Tm tm;
	char *s, *ss;

	if(m->unixDate != nil)
		return 1;

	if(!top){
bogus:
		m->unixDate = estrdup("");
		m->unixFrom = unixFrom(nil);
		return 1;
	}

	if(msgReadFile(m, "unixheader", &ss) < 0)
		return 0;
	s = ss;
	s = strchr(s, ' ');
	if(s == nil){
		free(ss);
		goto bogus;
	}
	s++;
	m->unixFrom = unixFrom(s);
	s = (char*)headStr;
	if(date2tm(&tm, s) == nil)
		s = m->info[IUnixDate];
	if(s == nil){
		free(ss);
		goto bogus;
	}
	m->unixDate = estrdup(s);
	free(ss);
	return 1;
}

/*
 * parse the address in the unix header
 * last line of defence, so must return something
 */
static MAddr *
unixFrom(char *s)
{
	MAddr *a;
	char *e, *t;

	if(s == nil)
		return nil;
	headStr = (uchar*)s;
	t = emalloc(strlen(s) + 2);
	e = headAddrSpec(t, nil);
	if(e == nil)
		a = nil;
	else{
		if(*e != '\0')
			*e++ = '\0';
		else
			e = site;
		a = MKZ(MAddr);

		a->box = estrdup(t);
		a->host = estrdup(e);
	}
	free(t);
	return a;
}

/*
 * read in the entire header,
 * and parse out any existing mime headers
 */
static int
msgHeader(Msg *m, Header *h, char *file)
{
	char *s, *ss, *t, *te;
	ulong lines, n, nn;
	long ns;
	int dated, c;

	if(h->buf != nil)
		return 1;

	ns = msgReadFile(m, file, &ss);
	if(ns < 0)
		return 0;
	s = ss;
	n = ns;

	/*
	 * count lines ending with \n and \r\n
	 * add an extra line at the end, since upas/fs headers
	 * don't have a terminating \r\n
	 */
	lines = 1;
	te = s + ns;
	for(t = s; t < te; t++){
		c = *t;
		if(c == '\0')
			return msgBogus(m, BogusHeader);
		if(c != '\n')
			continue;
		if(t == s || t[-1] != '\r')
			n++;
		lines++;
	}
	if(t > s && t[-1] != '\n'){
		if(t[-1] != '\r')
			n++;
		n++;
	}
	n += 2;
	h->buf = emalloc(n + 1);
	h->size = n;
	h->lines = lines;

	/*
	 * make sure all headers end in \r\n
	 */
	nn = 0;
	for(t = s; t < te; t++){
		c = *t;
		if(c == '\n'){
			if(!nn || h->buf[nn - 1] != '\r')
				h->buf[nn++] = '\r';
			lines++;
		}
		h->buf[nn++] = c;
	}
	if(nn && h->buf[nn-1] != '\n'){
		if(h->buf[nn-1] != '\r')
			h->buf[nn++] = '\r';
		h->buf[nn++] = '\n';
	}
	h->buf[nn++] = '\r';
	h->buf[nn++] = '\n';
	h->buf[nn] = '\0';
	if(nn != n)
		bye("misconverted header %ld %ld", nn, n);
	free(s);

	/*
	 * and parse some mime headers
	 */
	headStr = (uchar*)h->buf;
	dated = 0;
	while(s = headAtom(headFieldStop)){
		if(cistrcmp(s, "content-type") == 0)
			mimeType(h);
		else if(cistrcmp(s, "content-transfer-encoding") == 0)
			mimeEncoding(h);
		else if(cistrcmp(s, "content-id") == 0)
			mimeId(h);
		else if(cistrcmp(s, "content-description") == 0)
			mimeDescription(h);
		else if(cistrcmp(s, "content-disposition") == 0)
			mimeDisposition(h);
		else if(cistrcmp(s, "content-md5") == 0)
			mimeMd5(h);
		else if(cistrcmp(s, "content-language") == 0)
			mimeLanguage(h);
		else if(h == &m->head && cistrcmp(s, "from") == 0)
			m->from = headMAddr(m->from);
		else if(h == &m->head && cistrcmp(s, "to") == 0)
			m->to = headMAddr(m->to);
		else if(h == &m->head && cistrcmp(s, "reply-to") == 0)
			m->replyTo = headMAddr(m->replyTo);
		else if(h == &m->head && cistrcmp(s, "sender") == 0)
			m->sender = headMAddr(m->sender);
		else if(h == &m->head && cistrcmp(s, "cc") == 0)
			m->cc = headMAddr(m->cc);
		else if(h == &m->head && cistrcmp(s, "bcc") == 0)
			m->bcc = headMAddr(m->bcc);
		else if(h == &m->head && cistrcmp(s, "date") == 0)
			dated = 1;
		headSkip();
		free(s);
	}

	if(h == &m->head){
		if(m->from == nil){
			m->from = m->unixFrom;
			if(m->from != nil){
				s = maddrStr(m->from);
				msgAddHead(m, "From", s);
				free(s);
			}
		}
		if(m->sender == nil)
			m->sender = m->from;
		if(m->replyTo == nil)
			m->replyTo = m->from;

		if(infoIsNil(m->info[IDate]))
			m->info[IDate] = m->unixDate;
		if(!dated && m->from != nil)
			msgAddDate(m);
	}
	return 1;
}

/*
 * prepend head: body to the cached header
 */
static void
msgAddHead(Msg *m, char *head, char *body)
{
	char *s;
	long size, n;

	n = strlen(head) + strlen(body) + 4;
	size = m->head.size + n;
	s = emalloc(size + 1);
	snprint(s, size + 1, "%s: %s\r\n%s", head, body, m->head.buf);
	free(m->head.buf);
	m->head.buf = s;
	m->head.size = size;
	m->head.lines++;
}

static void
msgAddDate(Msg *m)
{
	Tm tm;
	char buf[64];

	/* don't bother if we don't have a date */
	if(infoIsNil(m->info[IDate]))
		return;

	date2tm(&tm, m->info[IDate]);
	rfc822date(buf, sizeof(buf), &tm);
	msgAddHead(m, "Date", buf);
}

static MimeHdr*
mkMimeHdr(char *s, char *t, MimeHdr *next)
{
	MimeHdr *mh;

	mh = MK(MimeHdr);
	mh->s = s;
	mh->t = t;
	mh->next = next;
	return mh;
}

static void
freeMimeHdr(MimeHdr *mh)
{
	MimeHdr *last;

	while(mh != nil){
		last = mh;
		mh = mh->next;
		free(last->s);
		free(last->t);
		free(last);
	}
}

static void
cleanupHeader(Header *h)
{
	freeMimeHdr(h->type);
	freeMimeHdr(h->id);
	freeMimeHdr(h->description);
	freeMimeHdr(h->encoding);
	freeMimeHdr(h->md5);
	freeMimeHdr(h->disposition);
	freeMimeHdr(h->language);
}

/*
 * parser for rfc822 & mime header fields
 */

/*
 * type		: 'content-type' ':' token '/' token params
 */
static void
mimeType(Header *h)
{
	char *s, *t;

	if(headChar(1) != ':')
		return;
	s = headAtom(mimeTokenStop);
	if(s == nil || headChar(1) != '/'){
		free(s);
		return;
	}
	t = headAtom(mimeTokenStop);
	if(t == nil){
		free(s);
		return;
	}
	h->type = mkMimeHdr(s, t, mimeParams());
}

/*
 * params	:
 *		| params ';' token '=' token
 * 		| params ';' token '=' quoted-str
 */
static MimeHdr*
mimeParams(void)
{
	MimeHdr head, *last;
	char *s, *t;

	head.next = nil;
	last = &head;
	for(;;){
		if(headChar(1) != ';')
			break;
		s = headAtom(mimeTokenStop);
		if(s == nil || headChar(1) != '='){
			free(s);
			break;
		}
		if(headChar(0) == '"'){
			t = headQuoted('"', '"');
			stripQuotes(t);
		}else
			t = headAtom(mimeTokenStop);
		if(t == nil){
			free(s);
			break;
		}
		last->next = mkMimeHdr(s, t, nil);
		last = last->next;
	}
	return head.next;
}

/*
 * encoding	: 'content-transfer-encoding' ':' token
 */
static void
mimeEncoding(Header *h)
{
	char *s;

	if(headChar(1) != ':')
		return;
	s = headAtom(mimeTokenStop);
	if(s == nil)
		return;
	h->encoding = mkMimeHdr(s, nil, nil);
}

/*
 * mailaddr	: ':' addresses
 */
static MAddr*
headMAddr(MAddr *old)
{
	MAddr *a;

	if(headChar(1) != ':')
		return old;

	if(headChar(0) == '\n')
		return old;

	a = headAddresses();
	if(a == nil)
		return old;

	freeMAddr(old);
	return a;
}

/*
 * addresses	: address | addresses ',' address
 */
static MAddr*
headAddresses(void)
{
	MAddr *addr, *tail, *a;

	addr = headAddress();
	if(addr == nil)
		return nil;
	tail = addr;
	while(headChar(0) == ','){
		headChar(1);
		a = headAddress();
		if(a == nil){
			freeMAddr(addr);
			return nil;
		}
		tail->next = a;
		tail = a;
	}
	return addr;
}

/*
 * address	: mailbox | group
 * group	: phrase ':' mboxes ';' | phrase ':' ';'
 * mailbox	: addr-spec
 *		| optphrase '<' addr-spec '>'
 *		| optphrase '<' route ':' addr-spec '>'
 * optphrase	: | phrase
 * route	: '@' domain
 *		| route ',' '@' domain
 * personal names are the phrase before '<',
 * or a comment before or after a simple addr-spec
 */
static MAddr*
headAddress(void)
{
	MAddr *addr;
	uchar *hs;
	char *s, *e, *w, *personal;
	int c;

	s = emalloc(strlen((char*)headStr) + 2);
	e = s;
	personal = headSkipWhite(1);
	c = headChar(0);
	if(c == '<')
		w = nil;
	else{
		w = headWord();
		c = headChar(0);
	}
	if(c == '.' || c == '@' || c == ',' || c == '\n' || c == '\0'){
		lastWhite = headStr;
		e = headAddrSpec(s, w);
		if(personal == nil){
			hs = headStr;
			headStr = lastWhite;
			personal = headSkipWhite(1);
			headStr = hs;
		}
	}else{
		if(c != '<' || w != nil){
			free(personal);
			if(!headPhrase(e, w)){
				free(s);
				return nil;
			}

			/*
			 * ignore addresses with groups,
			 * so the only thing left if <
			 */
			c = headChar(1);
			if(c != '<'){
				free(s);
				return nil;
			}
			personal = estrdup(s);
		}else
			headChar(1);

		/*
		 * after this point, we need to free personal before returning.
		 * set e to nil to everything afterwards fails.
		 *
		 * ignore routes, they are useless, and heavily discouraged in rfc1123.
		 * imap4 reports them up to, but not including, the terminating :
		 */
		e = s;
		c = headChar(0);
		if(c == '@'){
			for(;;){
				c = headChar(1);
				if(c != '@'){
					e = nil;
					break;
				}
				headDomain(e);
				c = headChar(1);
				if(c != ','){
					e = s;
					break;
				}
			}
			if(c != ':')
				e = nil;
		}

		if(e != nil)
			e = headAddrSpec(s, nil);
		if(headChar(1) != '>')
			e = nil;
	}

	/*
	 * e points to @host, or nil if an error occured
	 */
	if(e == nil){
		free(personal);
		addr = nil;
	}else{
		if(*e != '\0')
			*e++ = '\0';
		else
			e = site;
		addr = MKZ(MAddr);

		addr->personal = personal;
		addr->box = estrdup(s);
		addr->host = estrdup(e);
	}
	free(s);
	return addr;
}

/*
 * phrase	: word
 *		| phrase word
 * w is the optional initial word of the phrase
 * returns the end of the phrase, or nil if a failure occured
 */
static char*
headPhrase(char *e, char *w)
{
	int c;

	for(;;){
		if(w == nil){
			w = headWord();
			if(w == nil)
				return nil;
		}
		if(w[0] == '"')
			stripQuotes(w);
		strcpy(e, w);
		free(w);
		w = nil;
		e = strchr(e, '\0');
		c = headChar(0);
		if(c <= ' ' || strchr(headAtomStop, c) != nil && c != '"')
			break;
		*e++ = ' ';
		*e = '\0';
	}
	return e;
}

/*
 * addr-spec	: local-part '@' domain
 *		| local-part			extension to allow ! and local names
 * local-part	: word
 *		| local-part '.' word
 *
 * if no '@' is present, rewrite d!e!f!u as @d,@e:u@f,
 * where d, e, f are valid domain components.
 * the @d,@e: is ignored, since routes are ignored.
 * perhaps they should be rewritten as e!f!u@d, but that is inconsistent with upas.
 *
 * returns a pointer to '@', the end if none, or nil if there was an error
 */
static char*
headAddrSpec(char *e, char *w)
{
	char *s, *at, *b, *bang, *dom;
	int c;

	s = e;
	for(;;){
		if(w == nil){
			w = headWord();
			if(w == nil)
				return nil;
		}
		strcpy(e, w);
		free(w);
		w = nil;
		e = strchr(e, '\0');
		lastWhite = headStr;
		c = headChar(0);
		if(c != '.')
			break;
		headChar(1);
		*e++ = '.';
		*e = '\0';
	}

	if(c != '@'){
		/*
		 * extenstion: allow name without domain
		 * check for domain!xxx
		 */
		bang = domBang(s);
		if(bang == nil)
			return e;

		/*
		 * if dom1!dom2!xxx, ignore dom1!
		 */
		dom = s;
		for(; b = domBang(bang + 1); bang = b)
			dom = bang + 1;

		/*
		 * convert dom!mbox into mbox@dom
		 */
		*bang = '@';
		strrev(dom, bang);
		strrev(bang+1, e);
		strrev(dom, e);
		bang = &dom[e - bang - 1];
		if(dom > s){
			bang -= dom - s;
			for(e = s; *e = *dom; e++)
				dom++;
		}

		/*
		 * eliminate a trailing '.'
		 */
		if(e[-1] == '.')
			e[-1] = '\0';
		return bang;
	}
	headChar(1);

	at = e;
	*e++ = '@';
	*e = '\0';
	if(!headDomain(e))
		return nil;
	return at;
}

/*
 * find the ! in domain!rest, where domain must have at least
 * one internal '.'
 */
static char*
domBang(char *s)
{
	int dot, c;

	dot = 0;
	for(; c = *s; s++){
		if(c == '!'){
			if(!dot || dot == 1 && s[-1] == '.' || s[1] == '\0')
				return nil;
			return s;
		}
		if(c == '"')
			break;
		if(c == '.')
			dot++;
	}
	return nil;
}

/*
 * domain	: sub-domain
 *		| domain '.' sub-domain
 * returns the end of the domain, or nil if a failure occured
 */
static char*
headDomain(char *e)
{
	char *w;

	for(;;){
		w = headSubDomain();
		if(w == nil)
			return nil;
		strcpy(e, w);
		free(w);
		e = strchr(e, '\0');
		lastWhite = headStr;
		if(headChar(0) != '.')
			break;
		headChar(1);
		*e++ = '.';
		*e = '\0';
	}
	return e;
}

/*
 * id		: 'content-id' ':' msg-id
 * msg-id	: '<' addr-spec '>'
 */
static void
mimeId(Header *h)
{
	char *s, *e, *w;

	if(headChar(1) != ':')
		return;
	if(headChar(1) != '<')
		return;

	s = emalloc(strlen((char*)headStr) + 3);
	e = s;
	*e++ = '<';
	e = headAddrSpec(e, nil);
	if(e == nil || headChar(1) != '>'){
		free(s);
		return;
	}
	e = strchr(e, '\0');
	*e++ = '>';
	e[0] = '\0';
	w = strdup(s);
	free(s);
	h->id = mkMimeHdr(w, nil, nil);
}

/*
 * description	: 'content-description' ':' *text
 */
static void
mimeDescription(Header *h)
{
	if(headChar(1) != ':')
		return;
	headSkipWhite(0);
	h->description = mkMimeHdr(headText(), nil, nil);
}

/*
 * disposition	: 'content-disposition' ':' token params
 */
static void
mimeDisposition(Header *h)
{
	char *s;

	if(headChar(1) != ':')
		return;
	s = headAtom(mimeTokenStop);
	if(s == nil)
		return;
	h->disposition = mkMimeHdr(s, nil, mimeParams());
}

/*
 * md5		: 'content-md5' ':' token
 */
static void
mimeMd5(Header *h)
{
	char *s;

	if(headChar(1) != ':')
		return;
	s = headAtom(mimeTokenStop);
	if(s == nil)
		return;
	h->md5 = mkMimeHdr(s, nil, nil);
}

/*
 * language	: 'content-language' ':' langs
 * langs	: token
 *		| langs commas token
 * commas	: ','
 *		| commas ','
 */
static void
mimeLanguage(Header *h)
{
	MimeHdr head, *last;
	char *s;

	head.next = nil;
	last = &head;
	for(;;){
		s = headAtom(mimeTokenStop);
		if(s == nil)
			break;
		last->next = mkMimeHdr(s, nil, nil);
		last = last->next;
		while(headChar(0) != ',')
			headChar(1);
	}
	h->language = head.next;
}

/*
 * token	: 1*<char 33-255, except "()<>@,;:\\\"/[]?=" aka mimeTokenStop>
 * atom		: 1*<chars 33-255, except "()<>@,;:\\\".[]" aka headAtomStop>
 * note this allows 8 bit characters, which occur in utf.
 */
static char*
headAtom(char *disallowed)
{
	char *s;
	int c, ns, as;

	headSkipWhite(0);

	s = emalloc(StrAlloc);
	as = StrAlloc;
	ns = 0;
	for(;;){
		c = *headStr++;
		if(c <= ' ' || strchr(disallowed, c) != nil){
			headStr--;
			break;
		}
		s[ns++] = c;
		if(ns >= as){
			as += StrAlloc;
			s = erealloc(s, as);
		}
	}
	if(ns == 0){
		free(s);
		return 0;
	}
	s[ns] = '\0';
	return s;
}

/*
 * sub-domain	: atom | domain-lit
 */
static char *
headSubDomain(void)
{
	if(headChar(0) == '[')
		return headQuoted('[', ']');
	return headAtom(headAtomStop);
}

/*
 * word	: atom | quoted-str
 */
static char *
headWord(void)
{
	if(headChar(0) == '"')
		return headQuoted('"', '"');
	return headAtom(headAtomStop);
}

/*
 * q is a quoted string.  remove enclosing " and and \ escapes
 */
static void
stripQuotes(char *q)
{
	char *s;
	int c;

	if(q == nil)
		return;
	s = q++;
	while(c = *q++){
		if(c == '\\'){
			c = *q++;
			if(!c)
				return;
		}
		*s++ = c;
	}
	s[-1] = '\0';
}

/*
 * quoted-str	: '"' *(any char but '"\\\r', or '\' any char, or linear-white-space) '"'
 * domain-lit	: '[' *(any char but '[]\\\r', or '\' any char, or linear-white-space) ']'
 */
static char *
headQuoted(int start, int stop)
{
	char *s;
	int c, ns, as;

	if(headChar(1) != start)
		return nil;
	s = emalloc(StrAlloc);
	as = StrAlloc;
	ns = 0;
	s[ns++] = start;
	for(;;){
		c = *headStr;
		if(c == stop){
			headStr++;
			break;
		}
		if(c == '\0'){
			free(s);
			return nil;
		}
		if(c == '\r'){
			headStr++;
			continue;
		}
		if(c == '\n'){
			headStr++;
			while(*headStr == ' ' || *headStr == '\t' || *headStr == '\r' || *headStr == '\n')
				headStr++;
			c = ' ';
		}else if(c == '\\'){
			headStr++;
			s[ns++] = c;
			c = *headStr;
			if(c == '\0'){
				free(s);
				return nil;
			}
			headStr++;
		}else
			headStr++;
		s[ns++] = c;
		if(ns + 1 >= as){	/* leave room for \c or "0 */
			as += StrAlloc;
			s = erealloc(s, as);
		}
	}
	s[ns++] = stop;
	s[ns] = '\0';
	return s;
}

/*
 * headText	: contents of rest of header line
 */
static char *
headText(void)
{
	uchar *v;
	char *s;

	v = headStr;
	headToEnd();
	s = emalloc(headStr - v + 1);
	memmove(s, v, headStr - v);
	s[headStr - v] = '\0';
	return s;
}

/*
 * white space is ' ' '\t' or nested comments.
 * skip white space.
 * if com and a comment is seen,
 * return it's contents and stop processing white space.
 */
static char*
headSkipWhite(int com)
{
	char *s;
	int c, incom, as, ns;

	s = nil;
	as = StrAlloc;
	ns = 0;
	if(com)
		s = emalloc(StrAlloc);
	incom = 0;
	for(; c = *headStr; headStr++){
		switch(c){
		case ' ':
		case '\t':
		case '\r':
			c = ' ';
			break;
		case '\n':
			c = headStr[1];
			if(c != ' ' && c != '\t')
				goto breakout;
			c = ' ';
			break;
		case '\\':
			if(com && incom)
				s[ns++] = c;
			c = headStr[1];
			if(c == '\0')
				goto breakout;
			headStr++;
			break;
		case '(':
			incom++;
			if(incom == 1)
				continue;
			break;
		case ')':
			incom--;
			if(com && !incom){
				s[ns] = '\0';
				return s;
			}
			break;
		default:
			if(!incom)
				goto breakout;
			break;
		}
		if(com && incom && (c != ' ' || ns > 0 && s[ns-1] != ' ')){
			s[ns++] = c;
			if(ns + 1 >= as){	/* leave room for \c or 0 */
				as += StrAlloc;
				s = erealloc(s, as);
			}
		}
	}
breakout:;
	free(s);
	return nil;
}

/*
 * return the next non-white character
 */
static int
headChar(int eat)
{
	int c;

	headSkipWhite(0);
	c = *headStr;
	if(eat && c != '\0' && c != '\n')
		headStr++;
	return c;
}

static void
headToEnd(void)
{
	uchar *s;
	int c;

	for(;;){
		s = headStr;
		c = *s++;
		while(c == '\r')
			c = *s++;
		if(c == '\n'){
			c = *s++;
			if(c != ' ' && c != '\t')
				return;
		}
		if(c == '\0')
			return;
		headStr = s;
	}
}

static void
headSkip(void)
{
	int c;

	while(c = *headStr){
		headStr++;
		if(c == '\n'){
			c = *headStr;
			if(c == ' ' || c == '\t')
				continue;
			return;
		}
	}
}
