/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

uint16_t smbnhgets(uint8_t *);
uint32_t smbnhgetl(uint8_t *);
int64_t smbnhgetv(uint8_t *);
void smbhnputs(uint8_t *, uint16_t);
void smbhnputl(uint8_t *, uint32_t);
void smbhnputv(uint8_t *, int64_t);

SmbProcessResult smbnegotiate (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomsessionsetupandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomtreeconnectandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomtransaction (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomcheckdirectory (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomtransaction2 (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomecho (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomopenandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomcreate (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomopen (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomclose (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomreadandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomwriteandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomqueryinformation (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomfindclose2 (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomtreedisconnect (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomqueryinformation2 (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomdelete (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomflush (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomwrite (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomsetinformation2 (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomcreatedirectory (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomdeletedirectory (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomrename (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomlockingandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomsetinformation (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);
SmbProcessResult smbcomntcreateandx (SmbSession *, SmbHeader *, uint8_t *, SmbBuffer *);

void *smbemalloc(uint32_t size);
void *smbemallocz(uint32_t size, int clear);
void smberealloc(void **pp, uint32_t size);
char *smbestrdup(char *s);
void smbfree(void **pp);

int smbcheckwordcount(char *name, SmbHeader *h, uint16_t wordcount);
int smbcheckwordandbytecount(char *name, SmbHeader *h, uint16_t wordcount, uint8_t **bdatap, uint8_t **edatap);
int smbsendunicode(SmbPeerInfo *i);

char *smbstringdup(SmbHeader *h, uint8_t *base, uint8_t **bdatap, uint8_t *edata);
char *smbstrdup(uint8_t **bdatap, uint8_t *edata);
char *smbstrinline(uint8_t **bdatap, uint8_t *edata);
int smbstrlen(char *string);
int smbstringlen(SmbPeerInfo *i, char *string);
void smbstringprint(char **p, char *fmt, ...);

int smbucs2len(char *string);
int smbstringput(SmbPeerInfo *p, uint32_t flags, uint8_t *buf, uint16_t n,
		 uint16_t maxlen, char *string);
int smbstrput(uint32_t flags, uint8_t *buf, uint16_t n, uint16_t maxlen,
	      char *string);
int smbstrnput(uint8_t *buf, uint16_t n, uint16_t maxlen, char *string, uint16_t size, int upcase);
int smbucs2put(uint32_t flags, uint8_t *buf, uint16_t n, uint16_t maxlen,
	       char *string);

void smbresponseinit(SmbSession *s, uint16_t maxlen);
int smbresponsealignl2(SmbSession *s, int l2a);
uint16_t smbresponseoffset(SmbSession *s);
int smbresponseputheader(SmbSession *s, SmbHeader *h, uint8_t errclass, uint16_t error);
int smbresponseputandxheader(SmbSession *s, SmbHeader *h, uint16_t andxcommand,
			     uint32_t *andxoffsetfixup);
int smbresponseputb(SmbSession *s, uint8_t b);
int smbresponseputs(SmbSession *s, uint16_t b);
int smbresponseputl(SmbSession *s, uint32_t l);
int smbresponseoffsetputs(SmbSession *s, uint16_t offset, uint16_t b);
int smbresponseputstring(SmbSession *s, int mustalign, char *string);
int smbresponseputstr(SmbSession *s, char *string);
SmbProcessResult smbresponsesend(SmbSession *s);
int smbresponseputerror(SmbSession *s, SmbHeader *h, uint8_t errclass, uint16_t error);
int smbresponseskip(SmbSession *s, uint16_t amount);
uint16_t smbresponsespace(SmbSession *s);
void smbresponsereset(SmbSession *s);
int smbresponsecpy(SmbSession *s, uint8_t *data, uint16_t datalen);

void smbtreedisconnect(SmbSession *s, SmbTree *t);
void smbtreedisconnectbyid(SmbSession *s, uint16_t id);
SmbTree *smbtreeconnect(SmbSession *s, SmbService *serv);
int smbchaincommand(SmbSession *s, SmbHeader *h, uint32_t andxfixupoffset,
		    uint8_t cmd, uint16_t offset, SmbBuffer *b);

SmbService *smbservicefind(SmbSession *s, char *path, char *service, uint8_t *errclassp, uint16_t *errorp);
void smbserviceput(SmbService *serv);
void smbserviceget(SmbService *serv);

int smbrap2(SmbSession *s);

void smbglobalsguess(int client);

SmbBuffer *smbbuffernew(uint32_t maxlen);
void smbbufferfree(SmbBuffer **bp);
SmbBuffer *smbbufferinit(void *base, void *bdata, uint32_t blen);
uint32_t smbbufferwriteoffset(SmbBuffer *s);
uint32_t smbbufferwritemaxoffset(SmbBuffer *s);
uint32_t smbbufferreadoffset(SmbBuffer *s);
void *smbbufferwritepointer(SmbBuffer *s);
void *smbbufferreadpointer(SmbBuffer *s);
int smbbufferputheader(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p);
int smbbufferputandxheader(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p, uint8_t andxcommand,
			   uint32_t *andxoffsetfixup);
int smbbufferputb(SmbBuffer *s, uint8_t b);
int smbbufferputs(SmbBuffer *s, uint16_t b);
int smbbufferputl(SmbBuffer *s, uint32_t l);
int smbbufferoffsetputs(SmbBuffer *s, uint32_t offset, uint16_t b);
int smbbufferputstring(SmbBuffer *b, SmbPeerInfo *p, uint32_t flags,
		       char *string);
int smbbufferpututstring(SmbBuffer *b, SmbPeerInfo *p, int mustalign, char *string);
int smbbufferputucs2(SmbBuffer *b, int mustalign, char *string);
int smbbufferputstr(SmbBuffer *s, char *string);
int smbbufferputstrn(SmbBuffer *s, char *string, int len, int upcase);
int smbbufferputerror(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p, uint8_t errclass, uint16_t error);
int smbbufferskip(SmbBuffer *s, uint32_t amount);
uint32_t smbbufferspace(SmbBuffer *s);
void smbbufferreset(SmbBuffer *s);
int smbbufferputbytes(SmbBuffer *s, void *data, uint32_t datalen);
int smbbuffergetbytes(SmbBuffer *b, void *buf, uint32_t len);
void smbbuffersetreadlen(SmbBuffer *b, uint32_t len);
int smbbuffertrimreadlen(SmbBuffer *b, uint32_t len);
uint32_t smbbufferwritespace(SmbBuffer *b);
int smbbuffergets(SmbBuffer *b, uint16_t *sp);
int smbbuffergetstr(SmbBuffer *b, uint32_t flags, char **sp);
int smbbuffergetstrinline(SmbBuffer *b, char **sp);
int smbbuffergetstrn(SmbBuffer *b, uint16_t size, char **sp);
int smbbuffergetstring(SmbBuffer *b, SmbHeader *h, uint32_t flags, char **sp);
int smbbuffergetucs2(SmbBuffer *b, uint32_t flags, char **sp);
void *smbbufferpointer(SmbBuffer *b, uint32_t offset);
int smbbuffergetb(SmbBuffer *b, uint8_t *bp);
int smbbuffergetl(SmbBuffer *b, uint32_t *lp);
int smbbuffergetv(SmbBuffer *b, int64_t *vp);
uint32_t smbbufferreadspace(SmbBuffer *b);
void smbbufferwritelimit(SmbBuffer *b, uint32_t limit);
int smbbufferreadskipto(SmbBuffer *b, uint32_t offset);
int smbbufferpushreadlimit(SmbBuffer *b, uint32_t offset);
int smbbufferpopreadlimit(SmbBuffer *b);
int smbbufferalignl2(SmbBuffer *b, int al2);
int smbbufferwritebackup(SmbBuffer *b, uint32_t offset);
int smbbufferreadbackup(SmbBuffer *b, uint32_t offset);
int smbbufferfixuprelatives(SmbBuffer *b, uint32_t fixupoffset);
int smbbufferfixuprelativel(SmbBuffer *b, uint32_t fixupoffset);
int smbbufferfixuprelativeinclusivel(SmbBuffer *b, uint32_t fixupoffset);
int smbbufferfixupabsolutes(SmbBuffer *b, uint32_t fixupoffset);
int smbbufferfixupabsolutel(SmbBuffer *b, uint32_t fixupoffset);
int smbbufferfixupl(SmbBuffer *b, uint32_t fixupoffset, uint32_t val);
int smbbufferputv(SmbBuffer *b, int64_t v);
int smbbufferputack(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p);
int smbbufferfill(SmbBuffer *b, uint8_t c, uint32_t len);
int smbbufferoffsetgetb(SmbBuffer *b, uint32_t offset, uint8_t *bp);
int smbbuffercopy(SmbBuffer *to, SmbBuffer *from, uint32_t amount);
int smbbufferoffsetcopystr(SmbBuffer *b, uint32_t offset, char *buf,
			   int buflen, int *lenp);

SmbClient *smbconnect(char *to, char *share, char **errmsgp);
void smbclientfree(SmbClient *s);
int smbsuccess(SmbHeader *h, char **errmsgp);

int smbtransactiondecodeprimary(SmbTransaction *t, SmbHeader *h, uint8_t *pdata, SmbBuffer *b, char **errmsgp);
int smbtransactiondecodeprimary2(SmbTransaction *t, SmbHeader *h, uint8_t *pdata, SmbBuffer *b, char **errmsgp);
void smbtransactionfree(SmbTransaction *t);
int smbtransactionencoderesponse(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencoderesponse2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencodeprimary(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	uint8_t *wordcountp, uint16_t *bytecountp, char **errmsgp);
int smbtransactionencodeprimary2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	uint8_t *wordcountp, uint16_t *bytecountp, char **errmsgp);
int smbtransactiondecoderesponse(SmbTransaction *t, SmbHeader *h, uint8_t *pdata, SmbBuffer *b, char **errmsgp);
int smbtransactiondecoderesponse2(SmbTransaction *t, SmbHeader *h, uint8_t *pdata, SmbBuffer *b, char **errmsgp);
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
int32_t smbidmapadd(SmbIdMap *m, void *p);
void smbidmapremovebyid(SmbIdMap *m, int32_t id);
void smbidmapremove(SmbIdMap *m, void *thing);
void smbidmapfree(SmbIdMap **mp, SMBIDMAPAPPLYFN *free, void *magic);
void smbidmapapply(SmbIdMap *mp, SMBIDMAPAPPLYFN *free, void *magic);
void *smbidmapfind(SmbIdMap *m, int32_t id);
void smbidmapremoveif(SmbIdMap *m, int (*f)(void *p, void *arg), void *arg);

void smbsearchfree(SmbSearch **searchp);
void smbsearchclose(SmbSession *s, SmbSearch *search);
void smbsearchclosebyid(SmbSession *s, uint16_t sid);

void smbseterror(SmbSession *s, uint8_t errclass, uint16_t error);

void smbplan9time2datetime(uint32_t time, int tzoff, uint16_t *datep,
			   uint16_t *timep);
uint32_t smbdatetime2plan9time(uint16_t date, uint16_t time, int tzoff);
int64_t smbplan9time2time(uint32_t time);
uint32_t smbplan9time2utime(uint32_t time, int tzoff);
uint32_t smbutime2plan9time(uint32_t time, int tzoff);
uint32_t smbtime2plan9time(int64_t);
void smbpathcanon(char *path);
void smbpathsplit(char *path, char **dirp, char **namep);

uint16_t smbplan9mode2dosattr(uint32_t mode);
uint32_t smbdosattr2plan9wstatmode(uint32_t oldmode, uint16_t attr);
uint32_t smbdosattr2plan9mode(uint16_t attr);

uint32_t smbplan9length2size32(int64_t size);

void smbfileclose(SmbSession *s, SmbFile *f);

void smbloglock(void);
void smblogunlock(void);
int smblogvprint(int cmd, char *fmt, va_list ap);
int translogprint(int cmd, char *fmt, ...);
int smblogprint(int cmd, char *fmt, ...);
int smblogprintif(int v, char *fmt, ...);
void smblogdata(int cmd, int (*print)(int cmd, char *fmt, ...), void *p, int32_t data, int32_t limit);

SmbSharedFile *smbsharedfileget(Dir *d, int p9mode, int *sharep);
void smbsharedfileput(SmbFile *f, SmbSharedFile *sf, int share);
int smbsharedfilelock(SmbSharedFile *sf, SmbSession *s, uint16_t pid, int64_t base, int64_t limit);
int smbsharedfileunlock(SmbSharedFile *sf, SmbSession *s, uint16_t pid, int64_t base, int64_t limit);

int64_t smbl2roundupint64_t(int64_t v, int l2);

int smblistencifs(SMBCIFSACCEPTFN *accept);

int smbnetserverenum2(SmbClient *c, uint32_t stype, char *domain,
		      int *entriesp, SmbRapServerInfo1 **sip,
		      char **errmsgp);

int smbbuffergetheader(SmbBuffer *b, SmbHeader *h, uint8_t **parametersp, uint16_t *bytecountp);
int smbbuffergetandcheckheader(SmbBuffer *b, SmbHeader *h, uint8_t command, int response,
	uint8_t **pdatap, uint16_t *bytecountp, char **errmsgp);
int smbcheckheader(SmbHeader *h, uint8_t command, int response, char **errmsgp);
int smbcheckheaderdirection(SmbHeader *h, int response, char **errmsgp);

SmbDirCache *smbmkdircache(SmbTree *t, char *path);
void smbdircachefree(SmbDirCache **cp);

int smbmatch(char *name, Reprog *rep);
Reprog *smbmkrep(char *pattern);

int smbclientopen(SmbClient *c, uint16_t mode, char *name, uint8_t *errclassp, uint16_t *errorp, uint16_t *fid, uint16_t *attrp,
uint32_t *mtimep, uint32_t *sizep, uint16_t *accessallowedp, char **errmsgp);

Rune smbruneconvert(Rune r, uint32_t flags);

int smbslut(SmbSlut *table, char *name);
char *smbrevslut(SmbSlut *table, int val);

SmbProcessResult smbtruncatefile(SmbSession *s, SmbFile *f, int64_t offset);

#ifdef LEAK
#define smbemallocz(n, z) mallocz(n, z)
#define smbemalloc(n) malloc(n)
#define smbestrdup(p) strcpy(malloc(strlen(p) + 1), p)
#endif

int smbremovefile(SmbTree *t, char *dir, char *name);
int smbbrowsesendhostannouncement(char *name, uint32_t periodms,
				  uint32_t type, char *comment,
				  char **errmsgp);
