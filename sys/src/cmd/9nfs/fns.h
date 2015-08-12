/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

int argopt(int c);
int auth2unix(Auth*, Authunix*);
int authhostowner(Session*);
int canlock(Lock*);
void chat(char*, ...);
void chatsrv(char*);
int checkreply(Session*, char*);
int checkunixmap(Unixmap*);
void clog(char*, ...);
int clunkfid(Session*, Fid*);
int convM2sattr(void*, Sattr*);
int dir2fattr(Unixidmap*, Dir*, void*);
int error(Rpccall*, int);
void fidtimer(Session*, int32_t);
int garbage(Rpccall*, char*);
int getdom(uint32_t, char*, int);
int getticket(Session*, char*);
char* id2name(Unixid**, int);
void idprint(int, Unixid*);
void* listalloc(int32_t, int32_t);
void lock(Lock*);
void mnttimer(int32_t);
int name2id(Unixid**, char*);
Fid* newfid(Session*);
int32_t niwrite(int, void*, int32_t);
Unixidmap* pair2idmap(char*, uint32_t);
void panic(char*, ...);
void putfid(Session*, Fid*);
int readunixidmaps(char*);
Unixid* readunixids(char*, int);
Xfid* rpc2xfid(Rpccall*, Dir*);
int rpcM2S(void*, Rpccall*, int);
int rpcS2M(Rpccall*, int, void*);
void rpcprint(int, Rpccall*);
void server(int argc, char* argv[], int, Progmap*);
void setfid(Session*, Fid*);
Xfid* setuser(Xfile*, char*);
void showauth(Auth*);
void srvinit(int, char*, char*);
char* strfind(char*);
int string2S(void*, String*);
int strparse(char*, int, char**);
void strprint(int);
char* strstore(char*);
Waitmsg* system(char*, char**);
Waitmsg* systeml(char*, ...);
void unlock(Lock*);
int xfattach(Session*, char*, int);
Xfid* xfauth(Xfile*, String*);
void xfauthclose(Xfid*);
int32_t xfauthread(Xfid*, int32_t, unsigned char*, int32_t);
int xfauthremove(Xfid*, char*);
int32_t xfauthwrite(Xfid*, int32_t, unsigned char*, int32_t);
void xfclear(Xfid*);
void xfclose(Xfid*);
Xfid* xfid(char*, Xfile*, int);
Xfile* xfile(Qid*, void*, int);
int xfopen(Xfid*, int);
int xfpurgeuid(Session*, char*);
Xfile* xfroot(char*, int);
int xfstat(Xfid*, Dir*);
Xfid* xfwalkcr(int, Xfid*, String*, int32_t);
int xfwstat(Xfid*, Dir*);
int xmesg(Session*, int);
int xp2fhandle(Xfile*, Fhandle);
void xpclear(Xfile*);
