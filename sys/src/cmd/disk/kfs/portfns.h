void	accessdir(Iobuf*, Dentry*, int);
void	authfree(File*);
void	addfree(Device, long, Superb*);
long	balloc(Device, int, long);
void	bfree(Device, long, int);
int	byname(void*, void*);
int	byuid(void*, void*);
int	checkname(char*);
int	checktag(Iobuf*, int, long);
void 	cmd_user(void);
char*	cname(char*);
int	con_attach(int, char*, char*);
int	con_clone(int, int);
int	con_create(int, char*, int, int, long, int);
int	con_open(int, int);
int	con_path(int, char*);
int	con_read(int, char*, long, int);
int	con_remove(int);
int	con_stat(int, char*);
int	con_swap(int, int);
int	con_clri(int);
int	con_session(void);
int	con_walk(int, char*);
int	con_write(int, char*, long, int);
int	con_wstat(int, char*);
void	cprint(char*, ...);
void	datestr(char*, long);
void	dbufread(Iobuf*, Dentry*, long);
Qid	dentryqid(Dentry*);
int	devcmp(Device, Device);
Iobuf*	dnodebuf(Iobuf*, Dentry*, long, int);
Iobuf*	dnodebuf1(Iobuf*, Dentry*, long, int);
void	dofilter(Filter*);
int	doremove(File *, int);
void	dtrunc(Iobuf*, Dentry*);
void	exit(void);
Float	famd(Float, int, int, int);
int	fchar(void);
ulong	fdf(Float, int);
void	fileinit(Chan*);
void	sublockinit(void);
File*	filep(Chan*, int, int);
int	fname(char*);
void	formatinit(void);
void	freefp(File*);
void	freewp(Wpath*);
Filsys*	fsstr(char*);
Iobuf*	getbuf(Device, long, int);
Dentry*	getdir(Iobuf*, int);
long	getraddr(Device);
Wpath*	getwp(Wpath*);
void	hexdump(void*, int);
int	iaccess(File*, Dentry*, int);
long	indfetch(Iobuf*, Dentry*, long, long , int, int);
int	ingroup(int, int);
void	iobufinit(void);
int	leadgroup(int, int);
void	mkchallenge(Chan*);
void	mkqid(Qid*, Dentry*, int);
int	mkqidcmp(Qid*, Dentry*);
void	mkqid9p1(Qid9p1*, Qid*);
void	mkqid9p2(Qid*, Qid9p1*, int);
int	netserve(char*);
File*	newfp(Chan*);
Qid	newqid(Device);
void	newstart(void);
Wpath*	newwp(void);
int	oconvD2M(Dentry*, void*);
int	oconvM2D(void*, Dentry*);
int	ofcallfmt(Fmt*);
void	panic(char*, ...);
int	prime(long);
void	putbuf(Iobuf*);
void	putwp(Wpath*);
long	qidpathgen(Device*);
void	rootream(Device, long);
void	settag(Iobuf*, int, long);
void serve(Chan*);
void	serve9p1(Chan*, uchar*, int);
void	serve9p2(Chan*, uchar*, int);
void	strrand(void*, int);
int	strtouid(char*);
int	strtouid1(char*);
int	superok(Device, long, int);
void	superream(Device, long);
void	sync(char*);
int	syncblock(void);
int	Tfmt(Fmt*);
Tlock*	tlocked(Iobuf*, Dentry*);
void	uidtostr(char*,int);
void	uidtostr1(char*,int);

#pragma varargck	argpos	cprint	1
#pragma varargck	argpos	panic	1

#pragma varargck	type	"C"	Chan*
#pragma varargck	type	"D"	Device
#pragma varargck	type 	"A"	Filta
#pragma varargck	type	"G"	int
#pragma varargck	type	"T"	long
#pragma varargck	type	"F"	Fcall*

typedef struct Oldfcall Oldfcall;	/* needed for pragma */
#pragma varargck	type	"O"	Oldfcall*
