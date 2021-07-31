#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

enum
{
	HASHSIZE = 1019,
};

typedef struct Redir	Redir;
struct Redir
{
	Redir	*next;
	char	*pat;
	char	*repl;
};

static Redir *redirtab[HASHSIZE];
static Redir *vhosttab[HASHSIZE];
static char emptystring[1];

/* replacement field decorated with redirection modifiers? */
int
isdecorated(char *repl)
{
	return repl[0] == Modsilent || repl[0] == Modperm;
}

/* return replacement without redirection modifiers */
char *
undecorated(char *repl)
{
	while (isdecorated(repl))
		repl++;
	return repl;
}

static int
hashasu(char *key, int n)
{
        ulong h;

	h = 0;
        while(*key != 0)
                h = 65599*h + *(uchar*)key++;
        return h % n;
}

static void
insert(Redir **tab, char *pat, char *repl)
{
	Redir **l;
	Redir *srch;
	ulong hash;

	hash = hashasu(pat, HASHSIZE);
	for(l = &tab[hash]; *l; l = &(*l)->next)
		;
	*l = srch = ezalloc(sizeof(Redir));
	srch->pat = pat;
	srch->repl = repl;
	srch->next = 0;
}

static void
cleartab(Redir **tab)
{
	Redir *t;
	int i;

	for(i = 0; i < HASHSIZE; i++){
		while((t = tab[i]) != nil){
			tab[i] = t->next;
			free(t->pat);
			free(t->repl);
			free(t);
		}
	}
}

void
redirectinit(void)
{
	static Biobuf *b = nil;
	static Qid qid;
	char *file, *line, *s, *host, *repl, *field[3];
	static char pfx[] = "http://";

	file = "/sys/lib/httpd.rewrite";
	if(b != nil){
		if(updateQid(Bfildes(b), &qid) == 0)
			return;
		Bterm(b);
	}
	b = Bopen(file, OREAD);
	if(b == nil)
		sysfatal("can't read from %s", file);
	updateQid(Bfildes(b), &qid);

	cleartab(redirtab);
	cleartab(vhosttab);

	while((line = Brdline(b, '\n')) != nil){
		line[Blinelen(b)-1] = 0;
		s = strchr(line, '#');
		if(s != nil && (s == line || s[-1] == ' ' || s[-1] == '\t'))
			*s = '\0'; 	/* chop comment iff after whitespace */
		if(tokenize(line, field, nelem(field)) == 2){
			repl = undecorated(field[1]);
			if(strncmp(field[0], pfx, STRLEN(pfx)) == 0 &&
			   strncmp(repl, pfx, STRLEN(pfx)) != 0){
				/* url -> filename */
				host = field[0] + STRLEN(pfx);
				s = strrchr(host, '/');
				if(s)
					*s = 0;  /* chop trailing slash */

				insert(vhosttab, estrdup(host), estrdup(field[1]));
			}else{
				insert(redirtab, estrdup(field[0]), estrdup(field[1]));
			}
		}
	}
	syslog(0, HTTPLOG, "redirectinit pid=%d", getpid());
}

static Redir*
lookup(Redir **tab, char *pat)
{
	Redir *srch;
	ulong hash;

	hash = hashasu(pat,HASHSIZE);
	for(srch = tab[hash]; srch != nil; srch = srch->next)
		if(strcmp(pat, srch->pat) == 0)
			return srch;
	return nil;
}

static char*
prevslash(char *p, char *s)
{
	while(--s > p)
		if(*s == '/')
			break;
	return s;
}

char*
redirect(HConnect *hc, char *path)
{
	Redir *redir;
	char *s, *newpath, *repl;
	int c, n;

	for(s = strchr(path, '\0'); s > path; s = prevslash(path, s)){
		c = *s;
		*s = '\0';
		redir = lookup(redirtab, path);
		*s = c;
		if(redir != nil){
			repl = redir->repl;	/* leave decorations on */
			n = strlen(repl) + strlen(s) + 2 + UTFmax;
			newpath = halloc(hc, n);
			snprint(newpath, n, "%s%s", repl, s);
			return newpath;
		}
	}
	return nil;
}

/*
 * if host is virtual, return implicit prefix for URI within webroot.
 * if not, return empty string.
 * return value should not be freed by caller.
 */
char*
masquerade(char *host)
{
	Redir *redir;

	redir = lookup(vhosttab, host);
	if(redir == nil)
		return emptystring;
	return undecorated(redir->repl);
}
