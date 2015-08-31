/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * sorted by 4,/^$/|sort -bd +1
 */
int	fqid(int fd, Qid *qid);
int	BNList(Biobuf *b, NList *nl, char *sep);
int	BSList(Biobuf *b, SList *sl, char *sep);
int	BimapMimeParams(Biobuf *b, MimeHdr *mh);
int	Bimapaddr(Biobuf *b, MAddr *a);
int	Bimapdate(Biobuf *b, Tm *tm);
int	Bimapstr(Biobuf *b, char *s);
int	Brfc822date(Biobuf *b, Tm *tm);
int	appendSave(char *mbox, int flags, char *head, Biobuf *b, int32_t n);
void	bye(char *fmt, ...);
int	cdCreate(char *dir, char *file, int mode, uint32_t perm);
int	cdExists(char *dir, char *file);
Dir	*cdDirstat(char *dir, char *file);
int	cdDirwstat(char *dir, char *file, Dir *d);
int	cdOpen(char *dir, char *file, int mode);
int	cdRemove(char *dir, char *file);
MbLock	*checkBox(Box *box, int imped);
int	ciisprefix(char *pre, char *s);
int	cistrcmp(const char*, const char*);
int	cistrncmp(const char*, const char*, int);
char	*cistrstr(const char *s, const char *sub);
void	closeBox(Box *box, int opened);
void	closeImp(Box *box, MbLock *ml);
int	copyBox(char *from, char *to, int doremove);
int	copyCheck(Box *box, Msg *m, int uids, void *v);
int	copySave(Box *box, Msg *m, int uids, void *vs);
char	*cramauth(void);
int	createBox(char *mbox, int dir);
Tm	*date2tm(Tm *tm, char *date);
int	decmutf7(char *out, int nout, char *in);
int	deleteMsgs(Box *box);
void	debuglog(char *fmt, ...);
void	*emalloc(uint32_t);
int	emptyImp(char *mbox);
void	enableForwarding(void);
int	encmutf7(char *out, int nout, char *in);
void	*erealloc(void*, uint32_t);
char	*estrdup(char*);
int	expungeMsgs(Box *box, int send);
void	*ezmalloc(uint32_t);
void	fetchBodyFill(uint32_t n);
void	fetchBody(Msg *m, Fetch *f);
Pair	fetchBodyPart(Fetch *f, uint32_t size);
void	fetchBodyStr(Fetch *f, char *buf, uint32_t size);
void	fetchBodyStruct(Msg *m, Header *h, int extensions);
void	fetchEnvelope(Msg *m);
int	fetchMsg(Box *box, Msg *m, int uids, void *fetch);
Msg	*fetchSect(Msg *m, Fetch *f);
int	fetchSeen(Box *box, Msg *m, int uids, void *vf);
void	fetchStructExt(Header *h);
Msg	*findMsgSect(Msg *m, NList *sect);
int	forMsgs(Box *box, MsgSet *ms, uint32_t max, int uids,
		   int (*f)(Box*, Msg*, int, void*), void *rock);
void	freeMsg(Msg *m);
uint32_t	imap4DateTime(char *date);
int	imap4Date(Tm *tm, char *date);
int	imap4date(char *s, int n, Tm *tm);
int	imapTmp(void);
char	*impName(char *name);
int	infoIsNil(char *s);
int	isdotdot(char*);
int	isprefix(char *pre, char *s);
int	issuffix(char *suf, char *s);
int	listBoxes(char *cmd, char *ref, char *pat);
char	*loginauth(void);
int	lsubBoxes(char *cmd, char *ref, char *pat);
char	*maddrStr(MAddr *a);
uint32_t	mapFlag(char *name);
uint32_t	mapInt(NamedInt *map, char *name);
void	mbLockRefresh(MbLock *ml);
int	mbLocked(void);
MbLock	*mbLock(void);
void	mbUnlock(MbLock *ml);
char	*mboxName(char*);
Fetch	*mkFetch(int op, Fetch *next);
NList	*mkNList(uint32_t n, NList *next);
SList	*mkSList(char *s, SList *next);
Store	*mkStore(int sign, int op, int flags);
int	moveBox(char *from, char *to);
void	msgDead(Msg *m);
int	msgFile(Msg *m, char *f);
int	msgInfo(Msg *m);
int	msgIsMulti(Header *h);
int	msgIsRfc822(Header *h);
int	msgSeen(Box *box, Msg *m);
uint32_t	msgSize(Msg *m);
int	msgStruct(Msg *m, int top);
char	*mutf7str(char*);
int	myChdir(char *dir);
int	okMbox(char *mbox);
Box	*openBox(char *name, char *fsname, int writable);
int	openLocked(char *dir, char *file, int mode);
void	parseErr(char *msg);
AuthInfo	*passLogin(char*, char*);
char	*readFile(int fd);
void	resetCurDir(void);
Fetch	*revFetch(Fetch *f);
NList	*revNList(NList *s);
SList	*revSList(SList *s);
int	rfc822date(char *s, int n, Tm *tm);
int	searchMsg(Msg *m, Search *s);
int32_t	selectFields(char *dst, int32_t n, char *hdr, SList *fields, int matches);
void	sendFlags(Box *box, int uids);
void	setFlags(Box *box, Msg *m, int f);
void	setupuser(AuthInfo*);
int	storeMsg(Box *box, Msg *m, int uids, void *fetch);
char	*strmutf7(char*);
void	strrev(char *s, char *e);
int	subscribe(char *mbox, int how);
void	wrImpFlags(char *buf, int flags, int killRecent);
void	writeErr(void);
void	writeFlags(Biobuf *b, Msg *m, int recentOk);

#pragma	varargck argpos	bye		1
#pragma	varargck argpos	debuglog	1

#define	MK(t)		((t*)emalloc(sizeof(t)))
#define	MKZ(t)		((t*)ezmalloc(sizeof(t)))
#define	MKN(t,n)	((t*)emalloc((n)*sizeof(t)))
#define	MKNZ(t,n)	((t*)ezmalloc((n)*sizeof(t)))
#define	MKNA(t,at,n)	((t*)emalloc(sizeof(t) + (n)*sizeof(at)))

#define STRLEN(cs)	(sizeof(cs)-1)
