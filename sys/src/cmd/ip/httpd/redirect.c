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

#define AUTHPRE	"basic:"

static Redir *redirtab[HASHSIZE];
static Redir *authtab[HASHSIZE];

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
	char *file, *line, *s, *field[3];

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
	cleartab(authtab);

	while((line = Brdline(b, '\n')) != nil){
		line[Blinelen(b)-1] = 0;
		s = strchr(line, '#');
		if(s != nil)
			*s = '\0'; 	/* chop comment */
		if(tokenize(line, field, nelem(field)) == 2){
			if(strncmp(field[1], AUTHPRE, STRLEN(AUTHPRE)) == 0)
				insert(authtab, estrdup(field[0]), estrdup(field[1]+STRLEN(AUTHPRE)));
			else
				insert(redirtab, estrdup(field[0]), estrdup(field[1]));
		}
	}
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
authrealm(HConnect *hc, char *path)
{
	Redir *redir;
	char *s;
	int c;

	for(s = strchr(path, '\0'); s > path; s = prevslash(path, s)){
		c = *s;
		*s = '\0';
		redir = lookup(authtab, path);
		*s = c;
		if(redir != nil)
			return hstrdup(hc, redir->repl);
	}
	return nil;
}

char*
redirect(HConnect *hc, char *path)
{
	Redir *redir;
	char *s, *newpath;
	int c, n;

	for(s = strchr(path, '\0'); s > path; s = prevslash(path, s)){
		c = *s;
		*s = '\0';
		redir = lookup(redirtab, path);
		*s = c;
		if(redir != nil){
			n = strlen(redir->repl) + strlen(s+1) + 2 + UTFmax;
			newpath = halloc(hc, n);
			snprint(newpath, n, "%s/%s", redir->repl, &s[c == '/']);
			return newpath;
		}
	}
	return nil;
}
