#include <u.h>
#include <libc.h>
#include <sys.h>
#include <fcall.h>

char	buf[1048576];
#define	NARG	5
uintptr	arg[NARG];

/* system calls not defined in libc.h */
int	sysr1(void);
int	_stat(char*, char*);
int	_fstat(int, char*);
int	_errstr(char*);
int	_wstat(char*, char*);
int	_fwstat(int, char*);
int	_read(int, void*, int);
int	_write(int, void*, int);
int	_read9p(int, void*, int);
int	_write9p(int, void*, int);
int	brk_(void*);
int	_nfstat(int, void*, int);
int	_nstat(char*, void*, int);
int	_nfwstat(int, void*, int);
int	_nwstat(char*, void*, int);
int	_fsession(char*, void*, int);
int	_mount(int, char*, int, char*);
int	_wait(void*);

struct{
	char	*name;
	int	(*func)(...);
}tab[]={
#include "tab.h"
	0,		0
};

uintptr parse(char *);
void catch(void*, char*);

char*
xctime(ulong t)
{
	char *buf, *s;

	s = ctime(t);
	s[strlen(s)-1] = '\0';	/* remove newline */
	buf = malloc(512);
	if(buf == nil)
		sysfatal("can't malloc: %r");
	snprint(buf, 512, "%s (%lud)", s, t);
	return buf;
}


char*
lstime(long l)
{
	static char buf[32];
	char *t;
	long clk;

	clk = time(0);
	t = ctime(l);
	/* 6 months in the past or a day in the future */
	if(l<clk-180L*24*60*60 || clk+24L*60*60<l){
		memmove(buf, t+4, 7);		/* month and day */
		memmove(buf+7, t+23, 5);		/* year */
	}else
		memmove(buf, t+4, 12);		/* skip day of week */
	buf[12] = 0;
	return buf;
}

void
main(int argc, char *argv[])
{
	int i, j, c;
	int oflag, xflag, sflag;
	vlong r;
	Dir d;
	char strs[1024];
	char ebuf[1024];

	fmtinstall('M', dirmodefmt);

	oflag = 0;
	xflag = 0;
	sflag = 0;
	ARGBEGIN{
	case 'o':
		oflag++;
		break;
	case 's':
		sflag++;
		break;
	case 'x':
		xflag++;
		break;
	default:
		goto Usage;
	}ARGEND
	if(argc<1 || argc>1+NARG){
    Usage:
		fprint(2, "usage: syscall [-ox] entry [args; buf==1MB buffer]\n");
		fprint(2, "\tsyscall write 1 hello 5\n");
		fprint(2, "\tsyscall -o errstr buf 1024\n");
		fprint(2, "\tsyscall -[xs] stat file buf 1024\n");
		exits("usage");
	}
	for(i=1; i<argc; i++)
		arg[i-1] = parse(argv[i]);
	notify(catch);
	for(i=0; tab[i].name; i++)
		if(strcmp(tab[i].name, argv[0])==0){
			/* special case for seek, pread, pwrite; vlongs are problematic */
			if(strcmp(argv[0], "seek") == 0)
				r=seek(arg[0], strtoll(argv[2], 0, 0), arg[2]);
			else if(strcmp(argv[0], "pread") == 0)
				r=pread(arg[0], (void*)arg[1], arg[2], strtoll(argv[4], 0, 0));
			else if(strcmp(argv[0], "pwrite") == 0)
				r=pwrite(arg[0], (void*)arg[1], arg[2], strtoll(argv[4], 0, 0));
			else
				r=(*tab[i].func)(arg[0], arg[1], arg[2], arg[3], arg[4]);
			if(r == -1){
				errstr(ebuf, sizeof ebuf);
				fprint(2, "syscall: return %lld, error:%s\n", r, ebuf);
			}else{
				ebuf[0] = 0;
				fprint(2, "syscall: return %lld, no error\n", r);
			}
			if(oflag)
				print("%s\n", buf);
			if(xflag){
				for(j=0; j<r; j++){
					if(j%16 == 0)
						print("%.4x\t", j);
					c = buf[j]&0xFF;
					if('!'<=c && c<='~')
						print(" %c ", c);
					else
						print("%.2ux ", c);
					if(j%16 == 15)
						print("\n");
				}
				print("\n");
			}
			if(sflag && r > 0){
				r = convM2D((uchar*)buf, r, &d, strs);
				if(r <= BIT16SZ)
					print("short stat message\n");
				else{
					print("[%s] ", d.muid);
					print("(%.16llux %lud %.2ux) ", d.qid.path, d.qid.vers, d.qid.type);
					print("%M (%luo) ", d.mode, d.mode);
					print("%c %d ", d.type, d.dev);
					print("%s %s ", d.uid, d.gid);
					print("%lld ", d.length);
					print("%s ", lstime(d.mtime));
					print("%s\n", d.name);
					print("\tmtime: %s\n\tatime: %s\n", xctime(d.mtime), xctime(d.atime));
				}
			}
			exits(ebuf);
		}
	fprint(2, "syscall: %s not known\n", argv[0]);
	exits("unknown");
}

uintptr
parse(char *s)
{
	char *t;
	uintptr l;

	if(strcmp(s, "buf") == 0)
		return (uintptr)buf;
	
	l = strtoull(s, &t, 0);
	if(t>s && *t==0)
		return l;
	return (uintptr)s; 
}

void
catch(void *, char *msg)
{
	fprint(2, "syscall: received note='%s'\n", msg);
	noted(NDFLT);
}
