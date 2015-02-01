/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* buf.c */
void		initibuf(Ibuf*, Ioproc*, int);
int		readibuf(Ibuf*, char*, int);
void		unreadline(Ibuf*, char*);
int		readline(Ibuf*, char*, int);

/* client.c */
int		newclient(int);
void		closeclient(Client*);
void		clonectl(Ctl*);
int		ctlwrite(Req*, Ctl*, char*, char*);
int		clientctlwrite(Req*, Client*, char*, char*);
int		globalctlwrite(Req*, char*, char*);
void		ctlread(Req*, Client*);
void		globalctlread(Req*);
void		plumburl(char*, char*);

/* cookies.c */
void		cookieread(Req*);
void		cookiewrite(Req*);
void		cookieopen(Req*);
void		cookieclunk(Fid*);
void		initcookies(char*);
void		closecookies(void);
void		httpsetcookie(char*, char*, char*);
char*	httpcookies(char*, char*, int);

/* fs.c */
void		initfs(void);

/* http.c */
int		httpopen(Client*, Url*);
int		httpread(Client*, Req*);
void		httpclose(Client*);

/* io.c */
int		iotlsdial(Ioproc*, char*, char*, char*, int*, int);
int		ioprint(Ioproc*, int, char*, ...);
#pragma varargck argpos ioprint 3

/* plumb.c */
void	plumbinit(void);
void	plumbstart(void);
void	replumb(Client*);

/* url.c */
Url*		parseurl(char*, Url*);
void		freeurl(Url*);
void		rewriteurl(Url*);
int		seturlquery(Url*, char*);
Url*		copyurl(Url*);
char*	escapeurl(char*, int(*)(int));
char*	unescapeurl(char*);
void		initurl(void);

/* util.c */
char*	estrdup(char*);
char*	estrmanydup(char*, ...);
char*	estredup(char*, char*);
void*	emalloc(uint);
void*	erealloc(void*, uint);
char*	strlower(char*);
