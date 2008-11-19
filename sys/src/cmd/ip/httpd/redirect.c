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
	uint	flags;		/* generated from repl's decorations */
};

static Redir *redirtab[HASHSIZE];
static Redir *vhosttab[HASHSIZE];
static char emptystring[1];
/* these two arrays must be kept in sync */
static char decorations[] = { Modsilent, Modperm, Modsubord, Modonly, '\0' };
static uint redirflags[] = { Redirsilent, Redirperm, Redirsubord, Redironly, };

/* replacement field decorated with redirection modifiers? */
static int
isdecorated(char *repl)
{
	return strchr(decorations, repl[0]) != nil;
}

static uint
decor2flags(char *repl)
{
	uint flags;
	char *p;

	flags = 0;
	while ((p = strchr(decorations, *repl++)) != nil)
		flags |= redirflags[p - decorations];
	return flags;
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
	srch->flags = decor2flags(repl);
	srch->repl = undecorated(repl);
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
	char *file, *line, *s, *host, *field[3];
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
			if(strncmp(field[0], pfx, STRLEN(pfx)) == 0 &&
			   strncmp(undecorated(field[1]), pfx, STRLEN(pfx)) != 0){
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
lookup(Redir **tab, char *pat, int count)
{
	Redir *srch;
	ulong hash;

	hash = hashasu(pat,HASHSIZE);
	for(srch = tab[hash]; srch != nil; srch = srch->next)
		if(strcmp(pat, srch->pat) == 0) {
			/* only exact match wanted? */
			if (!(srch->flags & Redironly) || count == 0)
				return srch;
		}
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

/*
 * find the longest match of path against the redirection table,
 * chopping off the rightmost path component until success or
 * there's nothing left.  return a copy of the replacement string
 * concatenated with a slash and the portion of the path *not* matched.
 * So a match of /who/gre/some/stuff.html matched against
 *	/who/gre	http://gremlinsrus.org
 * returns
 *	http://gremlinsrus.org/some/stuff.html
 *
 * further flags: if Redironly, match only the named page and no
 * subordinate ones.  if Redirsubord, map the named patch and any
 * subordinate ones to the same replacement URL.
 */
char*
redirect(HConnect *hc, char *path, uint *flagp)
{
	Redir *redir;
	char *s, *newpath, *repl;
	int c, n, count;

	count = 0;
	for(s = strchr(path, '\0'); s > path; s = prevslash(path, s)){
		c = *s;
		*s = '\0';
		redir = lookup(redirtab, path, count++);
		*s = c;
		if(redir != nil){
			if (flagp)
				*flagp = redir->flags;
			repl = redir->repl;
			if(redir->flags & Redirsubord)
				/* don't append s, all matches map to repl */
				s = "";
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

	redir = lookup(vhosttab, host, 0);
	if(redir == nil)
		return emptystring;
	return redir->repl;
}
