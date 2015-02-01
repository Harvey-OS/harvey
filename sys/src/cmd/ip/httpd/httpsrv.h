/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct HSPriv	HSPriv;

enum
{
	HSTIMEOUT	= 15 * 60 * 1000,

	/* rewrite replacement field modifiers */
	Modsilent	= '@',	/* don't tell the browser about the redirect. */
	Modperm		= '=',	/* generate permanent redirection */
	Modsubord	= '*',	/* map page & all subordinates to same URL */
	Modonly		= '>',	/* match only this page, not subordinates */

	Redirsilent	= 1<<0,
	Redirperm	= 1<<1,
	Redirsubord	= 1<<2,
	Redironly	= 1<<3,
};

struct HSPriv
{
	char		*remotesys;
	char		*remoteserv;
};

extern	int		logall[3];
extern	char*		HTTPLOG;
extern	char*		webroot;
extern	char*		netdir;

#define 		STRLEN(s)	(sizeof(s)-1)

/* emem.c */
char			*estrdup(char*);
void*			ezalloc(ulong);

/* sendfd.c */
int			authcheck(HConnect *c);
int			checkreq(HConnect *c, HContent *type, HContent *enc, long mtime, char *etag);
int			etagmatch(int, HETag*, char*);
HRange			*fixrange(HRange *h, long length);
int			sendfd(HConnect *c, int fd, Dir *dir, HContent *type, HContent *enc);

/* content.c */
void			contentinit(void);
HContents		dataclass(HConnect *, char*, int);
int			updateQid(int, Qid*);
HContents		uriclass(HConnect *, char*);

/* anonymous.c */
void			anonymous(HConnect*);

/* hint.c */
void			hintprint(HConnect *hc, Hio*, char *, int, int);
void			statsinit(void);
void			urlcanon(char *url);
void			urlinit(void);

/* init.c */
HConnect*		init(int, char**);

vlong			Bfilelen(void*);

/* redirect.c */
void			redirectinit(void);
char*			redirect(HConnect *hc, char*, uint *);
char*			masquerade(char*);
char*			authrealm(HConnect *hc, char *path);
char			*undecorated(char *repl);

/* log.c */
void			logit(HConnect*, char*, ...);
#pragma	varargck	argpos	logit	2
void			writelog(HConnect*, char*, ...);
#pragma	varargck	argpos	writelog	2

/* authorize.c */
int authorize(HConnect*, char*);

char *webroot;
