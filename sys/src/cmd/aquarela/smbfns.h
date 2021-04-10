/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

u16 smbnhgets(u8 *);
u32 smbnhgetl(u8 *);
i64 smbnhgetv(u8 *);
void smbhnputs(u8 *, u16);
void smbhnputl(u8 *, u32);
void smbhnputv(u8 *, i64);

SmbProcessResult smbnegotiate (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomsessionsetupandx (SmbSession *, SmbHeader *, u8 *,
					 SmbBuffer *);
SmbProcessResult smbcomtreeconnectandx (SmbSession *, SmbHeader *, u8 *,
					SmbBuffer *);
SmbProcessResult smbcomtransaction (SmbSession *, SmbHeader *, u8 *,
				    SmbBuffer *);
SmbProcessResult smbcomcheckdirectory (SmbSession *, SmbHeader *, u8 *,
				       SmbBuffer *);
SmbProcessResult smbcomtransaction2 (SmbSession *, SmbHeader *, u8 *,
				     SmbBuffer *);
SmbProcessResult smbcomecho (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomopenandx (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomcreate (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomopen (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomclose (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomreadandx (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomwriteandx (SmbSession *, SmbHeader *, u8 *,
				  SmbBuffer *);
SmbProcessResult smbcomqueryinformation (SmbSession *, SmbHeader *, u8 *,
					 SmbBuffer *);
SmbProcessResult smbcomfindclose2 (SmbSession *, SmbHeader *, u8 *,
				   SmbBuffer *);
SmbProcessResult smbcomtreedisconnect (SmbSession *, SmbHeader *, u8 *,
				       SmbBuffer *);
SmbProcessResult smbcomqueryinformation2 (SmbSession *, SmbHeader *, u8 *,
					  SmbBuffer *);
SmbProcessResult smbcomdelete (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomflush (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomwrite (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomsetinformation2 (SmbSession *, SmbHeader *, u8 *,
					SmbBuffer *);
SmbProcessResult smbcomcreatedirectory (SmbSession *, SmbHeader *, u8 *,
					SmbBuffer *);
SmbProcessResult smbcomdeletedirectory (SmbSession *, SmbHeader *, u8 *,
					SmbBuffer *);
SmbProcessResult smbcomrename (SmbSession *, SmbHeader *, u8 *, SmbBuffer *);
SmbProcessResult smbcomlockingandx (SmbSession *, SmbHeader *, u8 *,
				    SmbBuffer *);
SmbProcessResult smbcomsetinformation (SmbSession *, SmbHeader *, u8 *,
				       SmbBuffer *);
SmbProcessResult smbcomntcreateandx (SmbSession *, SmbHeader *, u8 *,
				     SmbBuffer *);

void *smbemalloc(u32 size);
void *smbemallocz(u32 size, int clear);
void smberealloc(void **pp, u32 size);
char *smbestrdup(char *s);
void smbfree(void **pp);

int smbcheckwordcount(char *name, SmbHeader *h, u16 wordcount);
int smbcheckwordandbytecount(char *name, SmbHeader *h, u16 wordcount,
			     u8 **bdatap, u8 **edatap);
int smbsendunicode(SmbPeerInfo *i);

char *smbstringdup(SmbHeader *h, u8 *base, u8 **bdatap, u8 *edata);
char *smbstrdup(u8 **bdatap, u8 *edata);
char *smbstrinline(u8 **bdatap, u8 *edata);
int smbstrlen(char *string);
int smbstringlen(SmbPeerInfo *i, char *string);
void smbstringprint(char **p, char *fmt, ...);

int smbucs2len(char *string);
int smbstringput(SmbPeerInfo *p, u32 flags, u8 *buf, u16 n,
		 u16 maxlen, char *string);
int smbstrput(u32 flags, u8 *buf, u16 n, u16 maxlen,
	      char *string);
int smbstrnput(u8 *buf, u16 n, u16 maxlen, char *string,
	       u16 size, int upcase);
int smbucs2put(u32 flags, u8 *buf, u16 n, u16 maxlen,
	       char *string);

void smbresponseinit(SmbSession *s, u16 maxlen);
int smbresponsealignl2(SmbSession *s, int l2a);
u16 smbresponseoffset(SmbSession *s);
int smbresponseputheader(SmbSession *s, SmbHeader *h, u8 errclass,
			 u16 error);
int smbresponseputandxheader(SmbSession *s, SmbHeader *h, u16 andxcommand,
			     u32 *andxoffsetfixup);
int smbresponseputb(SmbSession *s, u8 b);
int smbresponseputs(SmbSession *s, u16 b);
int smbresponseputl(SmbSession *s, u32 l);
int smbresponseoffsetputs(SmbSession *s, u16 offset, u16 b);
int smbresponseputstring(SmbSession *s, int mustalign, char *string);
int smbresponseputstr(SmbSession *s, char *string);
SmbProcessResult smbresponsesend(SmbSession *s);
int smbresponseputerror(SmbSession *s, SmbHeader *h, u8 errclass,
			u16 error);
int smbresponseskip(SmbSession *s, u16 amount);
u16 smbresponsespace(SmbSession *s);
void smbresponsereset(SmbSession *s);
int smbresponsecpy(SmbSession *s, u8 *data, u16 datalen);

void smbtreedisconnect(SmbSession *s, SmbTree *t);
void smbtreedisconnectbyid(SmbSession *s, u16 id);
SmbTree *smbtreeconnect(SmbSession *s, SmbService *serv);
int smbchaincommand(SmbSession *s, SmbHeader *h, u32 andxfixupoffset,
		    u8 cmd, u16 offset, SmbBuffer *b);

SmbService *smbservicefind(SmbSession *s, char *path, char *service,
			   u8 *errclassp, u16 *errorp);
void smbserviceput(SmbService *serv);
void smbserviceget(SmbService *serv);

int smbrap2(SmbSession *s);

void smbglobalsguess(int client);

SmbBuffer *smbbuffernew(u32 maxlen);
void smbbufferfree(SmbBuffer **bp);
SmbBuffer *smbbufferinit(void *base, void *bdata, u32 blen);
u32 smbbufferwriteoffset(SmbBuffer *s);
u32 smbbufferwritemaxoffset(SmbBuffer *s);
u32 smbbufferreadoffset(SmbBuffer *s);
void *smbbufferwritepointer(SmbBuffer *s);
void *smbbufferreadpointer(SmbBuffer *s);
int smbbufferputheader(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p);
int smbbufferputandxheader(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p,
			   u8 andxcommand,
			   u32 *andxoffsetfixup);
int smbbufferputb(SmbBuffer *s, u8 b);
int smbbufferputs(SmbBuffer *s, u16 b);
int smbbufferputl(SmbBuffer *s, u32 l);
int smbbufferoffsetputs(SmbBuffer *s, u32 offset, u16 b);
int smbbufferputstring(SmbBuffer *b, SmbPeerInfo *p, u32 flags,
		       char *string);
int smbbufferpututstring(SmbBuffer *b, SmbPeerInfo *p, int mustalign, char *string);
int smbbufferputucs2(SmbBuffer *b, int mustalign, char *string);
int smbbufferputstr(SmbBuffer *s, char *string);
int smbbufferputstrn(SmbBuffer *s, char *string, int len, int upcase);
int smbbufferputerror(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p, u8 errclass,
		      u16 error);
int smbbufferskip(SmbBuffer *s, u32 amount);
u32 smbbufferspace(SmbBuffer *s);
void smbbufferreset(SmbBuffer *s);
int smbbufferputbytes(SmbBuffer *s, void *data, u32 datalen);
int smbbuffergetbytes(SmbBuffer *b, void *buf, u32 len);
void smbbuffersetreadlen(SmbBuffer *b, u32 len);
int smbbuffertrimreadlen(SmbBuffer *b, u32 len);
u32 smbbufferwritespace(SmbBuffer *b);
int smbbuffergets(SmbBuffer *b, u16 *sp);
int smbbuffergetstr(SmbBuffer *b, u32 flags, char **sp);
int smbbuffergetstrinline(SmbBuffer *b, char **sp);
int smbbuffergetstrn(SmbBuffer *b, u16 size, char **sp);
int smbbuffergetstring(SmbBuffer *b, SmbHeader *h, u32 flags, char **sp);
int smbbuffergetucs2(SmbBuffer *b, u32 flags, char **sp);
void *smbbufferpointer(SmbBuffer *b, u32 offset);
int smbbuffergetb(SmbBuffer *b, u8 *bp);
int smbbuffergetl(SmbBuffer *b, u32 *lp);
int smbbuffergetv(SmbBuffer *b, i64 *vp);
u32 smbbufferreadspace(SmbBuffer *b);
void smbbufferwritelimit(SmbBuffer *b, u32 limit);
int smbbufferreadskipto(SmbBuffer *b, u32 offset);
int smbbufferpushreadlimit(SmbBuffer *b, u32 offset);
int smbbufferpopreadlimit(SmbBuffer *b);
int smbbufferalignl2(SmbBuffer *b, int al2);
int smbbufferwritebackup(SmbBuffer *b, u32 offset);
int smbbufferreadbackup(SmbBuffer *b, u32 offset);
int smbbufferfixuprelatives(SmbBuffer *b, u32 fixupoffset);
int smbbufferfixuprelativel(SmbBuffer *b, u32 fixupoffset);
int smbbufferfixuprelativeinclusivel(SmbBuffer *b, u32 fixupoffset);
int smbbufferfixupabsolutes(SmbBuffer *b, u32 fixupoffset);
int smbbufferfixupabsolutel(SmbBuffer *b, u32 fixupoffset);
int smbbufferfixupl(SmbBuffer *b, u32 fixupoffset, u32 val);
int smbbufferputv(SmbBuffer *b, i64 v);
int smbbufferputack(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p);
int smbbufferfill(SmbBuffer *b, u8 c, u32 len);
int smbbufferoffsetgetb(SmbBuffer *b, u32 offset, u8 *bp);
int smbbuffercopy(SmbBuffer *to, SmbBuffer *from, u32 amount);
int smbbufferoffsetcopystr(SmbBuffer *b, u32 offset, char *buf,
			   int buflen, int *lenp);

SmbClient *smbconnect(char *to, char *share, char **errmsgp);
void smbclientfree(SmbClient *s);
int smbsuccess(SmbHeader *h, char **errmsgp);

int smbtransactiondecodeprimary(SmbTransaction *t, SmbHeader *h, u8 *pdata,
				SmbBuffer *b, char **errmsgp);
int smbtransactiondecodeprimary2(SmbTransaction *t, SmbHeader *h, u8 *pdata,
				 SmbBuffer *b, char **errmsgp);
void smbtransactionfree(SmbTransaction *t);
int smbtransactionencoderesponse(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencoderesponse2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencodeprimary(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	u8 *wordcountp, u16 *bytecountp, char **errmsgp);
int smbtransactionencodeprimary2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	u8 *wordcountp, u16 *bytecountp, char **errmsgp);
int smbtransactiondecoderesponse(SmbTransaction *t, SmbHeader *h, u8 *pdata,
				 SmbBuffer *b, char **errmsgp);
int smbtransactiondecoderesponse2(SmbTransaction *t, SmbHeader *h, u8 *pdata,
				  SmbBuffer *b, char **errmsgp);
int smbtransactionclientsend(void *magic, SmbBuffer *ob, char **errmsgp);
int smbtransactionclientreceive(void *magic, SmbBuffer *ib, char **errmsgp);

int smbtransactionexecute(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
	SmbBuffer *ob, SmbTransactionMethod *method, void *magic, SmbHeader *rh, char **errmsgp);
int smbtransactionrespond(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	SmbTransactionMethod *method, void *magic, char **errmsgp);

SmbProcessResult smbtrans2findfirst2(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2findnext2(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2queryfileinformation(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2queryfsinformation(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2querypathinformation(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2setfileinformation(SmbSession *s, SmbHeader *h);
SmbProcessResult smbtrans2setpathinformation(SmbSession *s, SmbHeader *h);

SmbIdMap *smbidmapnew(void);
i32 smbidmapadd(SmbIdMap *m, void *p);
void smbidmapremovebyid(SmbIdMap *m, i32 id);
void smbidmapremove(SmbIdMap *m, void *thing);
void smbidmapfree(SmbIdMap **mp, SMBIDMAPAPPLYFN *free, void *magic);
void smbidmapapply(SmbIdMap *mp, SMBIDMAPAPPLYFN *free, void *magic);
void *smbidmapfind(SmbIdMap *m, i32 id);
void smbidmapremoveif(SmbIdMap *m, int (*f)(void *p, void *arg), void *arg);

void smbsearchfree(SmbSearch **searchp);
void smbsearchclose(SmbSession *s, SmbSearch *search);
void smbsearchclosebyid(SmbSession *s, u16 sid);

void smbseterror(SmbSession *s, u8 errclass, u16 error);

void smbplan9time2datetime(u32 time, int tzoff, u16 *datep,
			   u16 *timep);
u32 smbdatetime2plan9time(u16 date, u16 time, int tzoff);
i64 smbplan9time2time(u32 time);
u32 smbplan9time2utime(u32 time, int tzoff);
u32 smbutime2plan9time(u32 time, int tzoff);
u32 smbtime2plan9time(i64);
void smbpathcanon(char *path);
void smbpathsplit(char *path, char **dirp, char **namep);

u16 smbplan9mode2dosattr(u32 mode);
u32 smbdosattr2plan9wstatmode(u32 oldmode, u16 attr);
u32 smbdosattr2plan9mode(u16 attr);

u32 smbplan9length2size32(i64 size);

void smbfileclose(SmbSession *s, SmbFile *f);

void smbloglock(void);
void smblogunlock(void);
int smblogvprint(int cmd, char *fmt, va_list ap);
int translogprint(int cmd, char *fmt, ...);
int smblogprint(int cmd, char *fmt, ...);
int smblogprintif(int v, char *fmt, ...);
void smblogdata(int cmd, int (*print)(int cmd, char *fmt, ...), void *p, i32 data, i32 limit);

SmbSharedFile *smbsharedfileget(Dir *d, int p9mode, int *sharep);
void smbsharedfileput(SmbFile *f, SmbSharedFile *sf, int share);
int smbsharedfilelock(SmbSharedFile *sf, SmbSession *s, u16 pid, i64 base, i64 limit);
int smbsharedfileunlock(SmbSharedFile *sf, SmbSession *s, u16 pid, i64 base, i64 limit);

i64 smbl2roundupi64(i64 v, int l2);

int smblistencifs(SMBCIFSACCEPTFN *accept);

int smbnetserverenum2(SmbClient *c, u32 stype, char *domain,
		      int *entriesp, SmbRapServerInfo1 **sip,
		      char **errmsgp);

int smbbuffergetheader(SmbBuffer *b, SmbHeader *h, u8 **parametersp,
		       u16 *bytecountp);
int smbbuffergetandcheckheader(SmbBuffer *b, SmbHeader *h, u8 command,
			       int response,
			       u8 **pdatap, u16 *bytecountp,
			       char **errmsgp);
int smbcheckheader(SmbHeader *h, u8 command, int response, char **errmsgp);
int smbcheckheaderdirection(SmbHeader *h, int response, char **errmsgp);

SmbDirCache *smbmkdircache(SmbTree *t, char *path);
void smbdircachefree(SmbDirCache **cp);

int smbmatch(char *name, Reprog *rep);
Reprog *smbmkrep(char *pattern);

int smbclientopen(SmbClient *c, u16 mode, char *name, u8 *errclassp,
		  u16 *errorp, u16 *fid, u16 *attrp,
u32 *mtimep, u32 *sizep, u16 *accessallowedp, char **errmsgp);

Rune smbruneconvert(Rune r, u32 flags);

int smbslut(SmbSlut *table, char *name);
char *smbrevslut(SmbSlut *table, int val);

SmbProcessResult smbtruncatefile(SmbSession *s, SmbFile *f, i64 offset);

#ifdef LEAK
#define smbemallocz(n, z) mallocz(n, z)
#define smbemalloc(n) malloc(n)
#define smbestrdup(p) strcpy(malloc(strlen(p) + 1), p)
#endif

int smbremovefile(SmbTree *t, char *dir, char *name);
int smbbrowsesendhostannouncement(char *name, u32 periodms,
				  u32 type, char *comment,
				  char **errmsgp);
