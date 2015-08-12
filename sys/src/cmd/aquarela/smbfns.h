/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

ushort smbnhgets(uchar *);
uint32_t smbnhgetl(uchar *);
vlong smbnhgetv(uchar *);
void smbhnputs(uchar *, ushort);
void smbhnputl(uchar *, uint32_t);
void smbhnputv(uchar *, vlong);

SMBPROCESSFN smbnegotiate;
SMBPROCESSFN smbcomsessionsetupandx;
SMBPROCESSFN smbcomtreeconnectandx;
SMBPROCESSFN smbcomtransaction;
SMBPROCESSFN smbcomcheckdirectory;
SMBPROCESSFN smbcomtransaction2;
SMBPROCESSFN smbcomecho;
SMBPROCESSFN smbcomopenandx;
SMBPROCESSFN smbcomcreate;
SMBPROCESSFN smbcomopen;
SMBPROCESSFN smbcomclose;
SMBPROCESSFN smbcomreadandx;
SMBPROCESSFN smbcomwriteandx;
SMBPROCESSFN smbcomqueryinformation;
SMBPROCESSFN smbcomfindclose2;
SMBPROCESSFN smbcomtreedisconnect;
SMBPROCESSFN smbcomqueryinformation2;
SMBPROCESSFN smbcomdelete;
SMBPROCESSFN smbcomflush;
SMBPROCESSFN smbcomwrite;
SMBPROCESSFN smbcomsetinformation2;
SMBPROCESSFN smbcomcreatedirectory;
SMBPROCESSFN smbcomdeletedirectory;
SMBPROCESSFN smbcomrename;
SMBPROCESSFN smbcomlockingandx;
SMBPROCESSFN smbcomsetinformation;
SMBPROCESSFN smbcomntcreateandx;

void *smbemalloc(uint32_t size);
void *smbemallocz(uint32_t size, int clear);
void smberealloc(void **pp, uint32_t size);
char *smbestrdup(char *s);
void smbfree(void **pp);

int smbcheckwordcount(char *name, SmbHeader *h, ushort wordcount);
int smbcheckwordandbytecount(char *name, SmbHeader *h, ushort wordcount, uchar **bdatap, uchar **edatap);
int smbsendunicode(SmbPeerInfo *i);

char *smbstringdup(SmbHeader *h, uchar *base, uchar **bdatap, uchar *edata);
char *smbstrdup(uchar **bdatap, uchar *edata);
char *smbstrinline(uchar **bdatap, uchar *edata);
int smbstrlen(char *string);
int smbstringlen(SmbPeerInfo *i, char *string);
void smbstringprint(char **p, char *fmt, ...);

int smbucs2len(char *string);
int smbstringput(SmbPeerInfo *p, uint32_t flags, uchar *buf, ushort n,
		 ushort maxlen, char *string);
int smbstrput(uint32_t flags, uchar *buf, ushort n, ushort maxlen,
	      char *string);
int smbstrnput(uchar *buf, ushort n, ushort maxlen, char *string, ushort size, int upcase);
int smbucs2put(uint32_t flags, uchar *buf, ushort n, ushort maxlen,
	       char *string);

void smbresponseinit(SmbSession *s, ushort maxlen);
int smbresponsealignl2(SmbSession *s, int l2a);
ushort smbresponseoffset(SmbSession *s);
int smbresponseputheader(SmbSession *s, SmbHeader *h, uchar errclass, ushort error);
int smbresponseputandxheader(SmbSession *s, SmbHeader *h, ushort andxcommand,
			     uint32_t *andxoffsetfixup);
int smbresponseputb(SmbSession *s, uchar b);
int smbresponseputs(SmbSession *s, ushort s);
int smbresponseputl(SmbSession *s, uint32_t l);
int smbresponseoffsetputs(SmbSession *s, ushort offset, ushort s);
int smbresponseputstring(SmbSession *s, int mustalign, char *string);
int smbresponseputstr(SmbSession *s, char *string);
SmbProcessResult smbresponsesend(SmbSession *s);
int smbresponseputerror(SmbSession *s, SmbHeader *h, uchar errclass, ushort error);
int smbresponseskip(SmbSession *s, ushort amount);
ushort smbresponsespace(SmbSession *s);
void smbresponsereset(SmbSession *s);
int smbresponsecpy(SmbSession *s, uchar *data, ushort datalen);

void smbtreedisconnect(SmbSession *s, SmbTree *t);
void smbtreedisconnectbyid(SmbSession *s, ushort id);
SmbTree *smbtreeconnect(SmbSession *s, SmbService *serv);
int smbchaincommand(SmbSession *s, SmbHeader *h, uint32_t andxfixupoffset,
		    uchar cmd, ushort offset, SmbBuffer *b);

SmbService *smbservicefind(SmbSession *s, char *path, char *service, uchar *errclassp, ushort *errorp);
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
int smbbufferputandxheader(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p, uchar andxcommand,
			   uint32_t *andxoffsetfixup);
int smbbufferputb(SmbBuffer *s, uchar b);
int smbbufferputs(SmbBuffer *s, ushort s);
int smbbufferputl(SmbBuffer *s, uint32_t l);
int smbbufferoffsetputs(SmbBuffer *s, uint32_t offset, ushort s);
int smbbufferputstring(SmbBuffer *b, SmbPeerInfo *p, uint32_t flags,
		       char *string);
int smbbufferpututstring(SmbBuffer *b, SmbPeerInfo *p, int mustalign, char *string);
int smbbufferputucs2(SmbBuffer *b, int mustalign, char *string);
int smbbufferputstr(SmbBuffer *s, char *string);
int smbbufferputstrn(SmbBuffer *s, char *string, int len, int upcase);
int smbbufferputerror(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p, uchar errclass, ushort error);
int smbbufferskip(SmbBuffer *s, uint32_t amount);
uint32_t smbbufferspace(SmbBuffer *s);
void smbbufferreset(SmbBuffer *s);
int smbbufferputbytes(SmbBuffer *s, void *data, uint32_t datalen);
int smbbuffergetbytes(SmbBuffer *b, void *buf, uint32_t len);
void smbbuffersetreadlen(SmbBuffer *b, uint32_t len);
int smbbuffertrimreadlen(SmbBuffer *b, uint32_t len);
uint32_t smbbufferwritespace(SmbBuffer *b);
int smbbuffergets(SmbBuffer *b, ushort *sp);
int smbbuffergetstr(SmbBuffer *b, uint32_t flags, char **sp);
int smbbuffergetstrinline(SmbBuffer *b, char **sp);
int smbbuffergetstrn(SmbBuffer *b, ushort size, char **sp);
int smbbuffergetstring(SmbBuffer *b, SmbHeader *h, uint32_t flags, char **sp);
int smbbuffergetucs2(SmbBuffer *b, uint32_t flags, char **sp);
void *smbbufferpointer(SmbBuffer *b, uint32_t offset);
int smbbuffergetb(SmbBuffer *b, uchar *bp);
int smbbuffergetl(SmbBuffer *b, uint32_t *lp);
int smbbuffergetv(SmbBuffer *b, vlong *vp);
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
int smbbufferputv(SmbBuffer *b, vlong v);
int smbbufferputack(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p);
int smbbufferfill(SmbBuffer *b, uchar c, uint32_t len);
int smbbufferoffsetgetb(SmbBuffer *b, uint32_t offset, uchar *bp);
int smbbuffercopy(SmbBuffer *to, SmbBuffer *from, uint32_t amount);
int smbbufferoffsetcopystr(SmbBuffer *b, uint32_t offset, char *buf,
			   int buflen, int *lenp);

SmbClient *smbconnect(char *to, char *share, char **errmsgp);
void smbclientfree(SmbClient *s);
int smbsuccess(SmbHeader *h, char **errmsgp);

int smbtransactiondecodeprimary(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp);
int smbtransactiondecodeprimary2(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp);
void smbtransactionfree(SmbTransaction *t);
int smbtransactionencoderesponse(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencoderesponse2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp);
int smbtransactionencodeprimary(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
				uchar *wordcountp, ushort *bytecountp, char **errmsgp);
int smbtransactionencodeprimary2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
				 uchar *wordcountp, ushort *bytecountp, char **errmsgp);
int smbtransactiondecoderesponse(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp);
int smbtransactiondecoderesponse2(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp);
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
long smbidmapadd(SmbIdMap *m, void *p);
void smbidmapremovebyid(SmbIdMap *m, long id);
void smbidmapremove(SmbIdMap *m, void *thing);
void smbidmapfree(SmbIdMap **mp, SMBIDMAPAPPLYFN *free, void *magic);
void smbidmapapply(SmbIdMap *mp, SMBIDMAPAPPLYFN *free, void *magic);
void *smbidmapfind(SmbIdMap *m, long id);
void smbidmapremoveif(SmbIdMap *m, int (*f)(void *p, void *arg), void *arg);

void smbsearchfree(SmbSearch **searchp);
void smbsearchclose(SmbSession *s, SmbSearch *search);
void smbsearchclosebyid(SmbSession *s, ushort sid);

void smbseterror(SmbSession *s, uchar errclass, ushort error);

void smbplan9time2datetime(uint32_t time, int tzoff, ushort *datep,
			   ushort *timep);
uint32_t smbdatetime2plan9time(ushort date, ushort time, int tzoff);
vlong smbplan9time2time(uint32_t time);
uint32_t smbplan9time2utime(uint32_t time, int tzoff);
uint32_t smbutime2plan9time(uint32_t time, int tzoff);
uint32_t smbtime2plan9time(vlong);
void smbpathcanon(char *path);
void smbpathsplit(char *path, char **dirp, char **namep);

ushort smbplan9mode2dosattr(uint32_t mode);
uint32_t smbdosattr2plan9wstatmode(uint32_t oldmode, ushort attr);
uint32_t smbdosattr2plan9mode(ushort attr);

uint32_t smbplan9length2size32(vlong size);

void smbfileclose(SmbSession *s, SmbFile *f);

void smbloglock(void);
void smblogunlock(void);
int smblogvprint(int cmd, char *fmt, va_list ap);
int translogprint(int cmd, char *fmt, ...);
int smblogprint(int cmd, char *fmt, ...);
int smblogprintif(int v, char *fmt, ...);
void smblogdata(int cmd, int (*print)(int cmd, char *fmt, ...), void *p, long data, long limit);

SmbSharedFile *smbsharedfileget(Dir *d, int p9mode, int *sharep);
void smbsharedfileput(SmbFile *f, SmbSharedFile *sf, int share);
int smbsharedfilelock(SmbSharedFile *sf, SmbSession *s, ushort pid, vlong base, vlong limit);
int smbsharedfileunlock(SmbSharedFile *sf, SmbSession *s, ushort pid, vlong base, vlong limit);

vlong smbl2roundupvlong(vlong v, int l2);

int smblistencifs(SMBCIFSACCEPTFN *accept);

int smbnetserverenum2(SmbClient *c, uint32_t stype, char *domain,
		      int *entriesp, SmbRapServerInfo1 **sip,
		      char **errmsgp);

int smbbuffergetheader(SmbBuffer *b, SmbHeader *h, uchar **parametersp, ushort *bytecountp);
int smbbuffergetandcheckheader(SmbBuffer *b, SmbHeader *h, uchar command, int response,
			       uchar **pdatap, ushort *bytecountp, char **errmsgp);
int smbcheckheader(SmbHeader *h, uchar command, int response, char **errmsgp);
int smbcheckheaderdirection(SmbHeader *h, int response, char **errmsgp);

SmbDirCache *smbmkdircache(SmbTree *t, char *path);
void smbdircachefree(SmbDirCache **cp);

int smbmatch(char *name, Reprog *rep);
Reprog *smbmkrep(char *pattern);

int smbclientopen(SmbClient *c, ushort mode, char *name, uchar *errclassp, ushort *errorp, ushort *fid, ushort *attrp,
		  uint32_t *mtimep, uint32_t *sizep, ushort *accessallowedp, char **errmsgp);

Rune smbruneconvert(Rune r, uint32_t flags);

int smbslut(SmbSlut *table, char *name);
char *smbrevslut(SmbSlut *table, int val);

SmbProcessResult smbtruncatefile(SmbSession *s, SmbFile *f, vlong offset);

#ifdef LEAK
#define smbemallocz(n, z) mallocz(n, z)
#define smbemalloc(n) malloc(n)
#define smbestrdup(p) strcpy(malloc(strlen(p) + 1), p)
#endif

int smbremovefile(SmbTree *t, char *dir, char *name);
int smbbrowsesendhostannouncement(char *name, uint32_t periodms,
				  uint32_t type, char *comment,
				  char **errmsgp);
