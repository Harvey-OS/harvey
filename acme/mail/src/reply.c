#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <ctype.h>
#include <plumb.h>
#include "dat.h"

static int	replyid;

int
EQUAL(char *s, char *t)
{
	while(tolower(*s) == tolower(*t++))
		if(*s++ == '\0')
			return 1;
	return 0;
}

void
mkreply(Message *m, char *label, char *to)
{
	Message *r;
	char *dir, *t;

	r = emalloc(sizeof(Message));
	r->isreply = 1;
	if(m != nil)
		r->replyname = estrdup(m->name);
	r->next = replies.head;
	r->prev = nil;
	if(replies.head != nil)
		replies.head->prev = r;
	replies.head = r;
	if(replies.tail == nil)
		replies.tail = r;
	r->name = emalloc(strlen(mbox.name)+strlen(label)+10);
	sprint(r->name, "%s%s%d", mbox.name, label, ++replyid);
	r->w = newwindow();
	winname(r->w, r->name);
	wintagwrite(r->w, "|fmt Post", 5+4);
	r->tagposted = 1;
	threadcreate(mesgctl, r, STACK);
	winopenbody(r->w, OWRITE);
	if(to!=nil && to[0]!='\0')
		Bprint(r->w->body, "%s\n", to);
	if(m != nil){
		dir = estrstrdup(mbox.name, m->name);
		if(to == nil){
			/* Reply goes to replyto; Reply all goes to From and To and CC */
			if(strcmp(label, "Reply") == 0)
				Bprint(r->w->body, "To: %s\n", m->replyto);
			else{	/* Replyall */
				if(strlen(m->from) > 0)
					Bprint(r->w->body, "To: %s\n", m->from);
				if(strlen(m->to) > 0)
					Bprint(r->w->body, "To: %s\n", m->to);
				if(strlen(m->cc) > 0)
					Bprint(r->w->body, "CC: %s\n", m->cc);
			}
		}
		if(strlen(m->subject) > 0){
			t = "Subject: Re: ";
			if(strlen(m->subject) >= 3)
				if(tolower(m->subject[0])=='r' && tolower(m->subject[1])=='e' && m->subject[2]==':')
					t = "Subject: ";
			Bprint(r->w->body, "%s%s\n", t, m->subject);
		}
		Bprint(r->w->body, "Include: %sraw\n", dir);
		free(dir);
	}
	Bprint(r->w->body, "\n");
	if(m == nil)
		Bprint(r->w->body, "\n");
	winclosebody(r->w);
	if(m == nil)
		winselect(r->w, "0", 0);
	else
		winselect(r->w, "$", 0);
	winclean(r->w);
	windormant(r->w);
}

void
delreply(Message *m)
{
	if(m->next == nil)
		replies.tail = m->prev;
	else
		m->next->prev = m->prev;
	if(m->prev == nil)
		replies.head = m->next;
	else
		m->prev->next = m->next;
	mesgfreeparts(m);
	free(m);
}

enum
{
	NARGS		= 100,
	NARGCHAR	= 8*1024,
	EXECSTACK 	= STACK+(NARGS+1)*sizeof(char*)+NARGCHAR
};

struct Exec
{
	char		**argv;
	int		p[2];
	Channel	*sync;
};

/* copy argv to stack and free the incoming strings, so we don't leak argument vectors */
void
buildargv(char **inargv, char *argv[NARGS+1], char args[NARGCHAR])
{
	int i, n;
	char *s, *a;

	s = args;
	for(i=0; i<NARGS; i++){
		a = inargv[i];
		if(a == nil)
			break;
		n = strlen(a)+1;
		if((s-args)+n >= NARGCHAR)	/* too many characters */
			break;
		argv[i] = s;
		memmove(s, a, n);
		s += n;
		free(a);
	}
	argv[i] = nil;
}

void
execproc(void *v)
{
	struct Exec *e;
	int p[2];
	char *argv[NARGS+1], args[NARGCHAR];

	e = v;
	p[0] = e->p[0];
	p[1] = e->p[1];
	rfork(RFFDG);
	sendul(e->sync, 1);
	buildargv(e->argv, argv, args);
	free(e->argv);
	chanfree(e->sync);
	free(e);
	dup(p[0], 0);
	close(p[1]);
	procexec(nil, "/bin/upas/marshal", argv);
//threadprint(2, "exec: /bin/upas/marshal");
//{int i;
//for(i=0; argv[i]; i++) print(" '%s'", argv[i]);
//print("\n");
//}
//argv[0] = "cat";
//argv[1] = nil;
//procexec(nil, "/bin/cat", argv);
	threadprint(2, "Mail: can't exec %s: %r\n", argv[0]);
	threadexits("can't exec");
}

enum{
	ATTACH,
	BCC,
	CC,
	FROM,
	INCLUDE,
	TO,
};

char *headers[] = {
	"attach:",
	"bcc:",
	"cc:",
	"from:",
	"include:",
	"to:",
	nil,
};

int
whichheader(char *h)
{
	int i;

	for(i=0; headers[i]!=nil; i++)
		if(EQUAL(h, headers[i]))
			return i;
	return -1;
}

char *tolist[200];
char	*cclist[200];
char	*bcclist[200];
int ncc, nbcc, nto;
char	*attlist[200];
int	rfc822[200];

int
addressed(char *name)
{
	int i;

	for(i=0; i<nto; i++)
		if(strcmp(name, tolist[i]) == 0)
			return 1;
	for(i=0; i<ncc; i++)
		if(strcmp(name, cclist[i]) == 0)
			return 1;
	for(i=0; i<nbcc; i++)
		if(strcmp(name, bcclist[i]) == 0)
			return 1;
	return 0;
}

char*
skipbl(char *s, char *e)
{
	while(s < e){
		if(*s!=' ' && *s!='\t' && *s!=',')
			break;
		s++;
	}
	return s;
}

char*
findbl(char *s, char *e)
{
	while(s < e){
		if(*s==' ' || *s=='\t' || *s==',')
			break;
		s++;
	}
	return s;
}

/*
 * comma-separate possibly blank-separated strings in line; e points before newline
 */
void
commas(char *s, char *e)
{
	char *t;

	/* may have initial blanks */
	s = skipbl(s, e);
	while(s < e){
		s = findbl(s, e);
		if(s == e)
			break;
		t = skipbl(s, e);
		if(t == e)	/* no more words */
			break;
		/* patch comma */
		*s++ = ',';
		while(s < t)
			*s++ = ' ';
	}
}

void
mesgsend(Message *m)
{
	char *s, *body, *to;
	int i, j, h, n, natt, p[2];
	struct Exec *e;
	Channel *sync;
	int first, nfld, delit;
	char *copy, *fld[100];

	body = winreadbody(m->w, &n);
	/* assemble to: list from first line, to: line, and cc: line */
	nto = 0;
	natt = 0;
	ncc = 0;
	nbcc = 0;
	first = 1;
	to = body;
	for(;;){
		for(s=to; *s!='\n'; s++)
			if(*s == '\0'){
				free(body);
				return;
			}
		if(s++ == to)	/* blank line */
			break;
		/* make copy of line to tokenize */
		copy = emalloc(s-to);
		memmove(copy, to, s-to);
		copy[s-to-1] = '\0';
		nfld = tokenizec(copy, fld, nelem(fld), ", \t");
		if(nfld == 0){
			free(copy);
			break;
		}
		n -= s-to;
		switch(h = whichheader(fld[0])){
		case TO:
		case FROM:
			delit = 1;
			commas(to+strlen(fld[0]), s-1);
			for(i=1; i<nfld && nto<nelem(tolist); i++)
				if(!addressed(fld[i]))
					tolist[nto++] = estrdup(fld[i]);
			break;
		case BCC:
			delit = 1;
			commas(to+strlen(fld[0]), s-1);
			for(i=1; i<nfld && nbcc<nelem(bcclist); i++)
				if(!addressed(fld[i]))
					bcclist[nbcc++] = estrdup(fld[i]);
			break;
		case CC:
			delit = 1;
			commas(to+strlen(fld[0]), s-1);
			for(i=1; i<nfld && ncc<nelem(cclist); i++)
				if(!addressed(fld[i]))
					cclist[ncc++] = estrdup(fld[i]);
			break;
		case ATTACH:
		case INCLUDE:
			delit = 1;
			for(i=1; i<nfld && natt<nelem(attlist); i++){
				attlist[natt] = estrdup(fld[i]);
				rfc822[natt++] = (h == INCLUDE);
			}
			break;
		default:
			if(first){
				delit = 1;
				for(i=0; i<nfld && nto<nelem(tolist); i++)
					tolist[nto++] = estrdup(fld[i]);
			}else	/* ignore it */
				delit = 0;
			break;
		}
		if(delit){
			/* delete line from body */
			memmove(to, s, n+1);
		}else
			to = s;
		free(copy);
		first = 0;
	}

	e = emalloc(sizeof(struct Exec));
	if(pipe(p) < 0)
		error("Mail: can't create pipe\n");
	e->p[0] = p[0];
	e->p[1] = p[1];
	e->argv = emalloc((1+1+4*natt+1)*sizeof(char*));
	e->argv[0] = estrdup("marshal");
	e->argv[1] = estrdup("-8");
	j = 2;
	for(i=0; i<natt; i++){
		if(rfc822[i]){
			e->argv[j++] = estrdup("-t");
			e->argv[j++] = estrdup("message/rfc822");
			e->argv[j++] = estrdup("-A");
		}else
			e->argv[j++] = estrdup("-a");
		e->argv[j++] = attlist[i];
	}
	sync = chancreate(sizeof(int), 0);
	e->sync = sync;
	proccreate(execproc, e, EXECSTACK);
	recvul(sync);
	close(p[0]);

	/* using marshal -8, so generate rfc822 headers */
	if(nto > 0){
		threadprint(p[1], "To: ");
		for(i=0; i<nto-1; i++)
			threadprint(p[1], "%s, ", tolist[i]);
		threadprint(p[1], "%s\n", tolist[i]);
	}
	if(ncc > 0){
		threadprint(p[1], "CC: ");
		for(i=0; i<ncc-1; i++)
			threadprint(p[1], "%s, ", cclist[i]);
		threadprint(p[1], "%s\n", cclist[i]);
	}
	if(nbcc > 0){
		threadprint(p[1], "BCC: ");
		for(i=0; i<nbcc-1; i++)
			threadprint(p[1], "%s, ", bcclist[i]);
		threadprint(p[1], "%s\n", bcclist[i]);
	}

	i = strlen(body);
	if(i > 0)
		write(p[1], body, i);

	/* guarantee a blank line, to ensure attachments are separated from body */
	if(i==0 || body[i-1]!='\n')
		write(p[1], "\n\n", 2);
	else if(i>1 && body[i-2]!='\n')
		write(p[1], "\n", 1);
	close(p[1]);
	free(body);

	if(m->replyname != nil)
		mesgmenumark(mbox.w, m->replyname, "\t[replied]");
	if(m->name[0] == '/')
		s = estrdup(m->name);
	else
		s = estrstrdup(mbox.name, m->name);
	s = egrow(s, "-R", nil);
	winname(m->w, s);
	free(s);
	winclean(m->w);
}
