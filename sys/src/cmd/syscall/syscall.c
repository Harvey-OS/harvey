#include <u.h>
#include <libc.h>

char	buf[8192];
#define	NARG	5
int	arg[NARG];

int	sysr1(void);

struct{
	char	*name;
	int	(*func)(...);
}tab[]={
#include "tab.h"
	0,		0
};

int
main(int argc, char *argv[])
{
	int i, r;
	int oflag=0;
	int parse(char *);
	char ebuf[ERRLEN];

	ARGBEGIN{
	case 'o':
		oflag++;
		break;
	default:
		goto Usage;
	}ARGEND
	if(argc<1 || argc>1+NARG){
    Usage:
		fprint(2, "usage: syscall [-o] entry [args; buf==1024 byte buffer]\n");
		fprint(2, "\tsyscall write 1 hello 5\n");
		fprint(2, "\tsyscall -o lasterr buf\n");
		return 1;
	}
	for(i=1; i<argc; i++)
		arg[i-1]=parse(argv[i]);
	for(i=0; tab[i].name; i++)
		if(strcmp(tab[i].name, argv[0])==0){
			r=(*tab[i].func)(arg[0], arg[1], arg[2], arg[3], arg[4]);
			if(r == -1){
				errstr(ebuf);
				fprint(2, "syscall: return %d, error:%s\n", r, ebuf);
			}else{
				ebuf[0] = 0;
				fprint(2, "syscall: return %d, no error\n", r);
			}
			if(oflag)
				print("%s\n", buf);
			exits(ebuf);
		}
	fprint(2, "syscall: %s not known\n", argv[0]);
	exits("unknown");
	return 0;		/* to keep compiler happy */
}
int
parse(char *s)
{
	char *t;
	long l;

	if(strcmp(s, "buf") == 0)
		return (int)buf;
	l = strtoul(s, &t, 0);
	if(t>s && *t==0)
		return l;
	return (int)s; 
}
