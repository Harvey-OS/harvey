/*
 * db - main command loop and error/interrupt handling
 */
#include "defs.h"
#include "fns.h"

int wtflag = OREAD;
BOOL kflag;

BOOL mkfault;
ADDR maxoff;

int xargc;		/* bullshit */

extern	BOOL	executing;
extern	int	infile;
int	exitflg;
extern	int	eof;

int	alldigs(char*);
void	fault(void*, char*);

extern	char	*Ipath;
jmp_buf env;
static char *errmsg;

void
main(int argc, char **argv)
{
	char b1[100];
	char b2[100];
	char *s;
	char *name;

	name = 0;
	outputinit();
	maxoff = MAXOFF;
	ARGBEGIN{
	case 'k':
		kflag = 1;
		break;
	case 'w':
		wtflag = ORDWR;		/* suitable for open() */
		break;
	case 'I':
		s = ARGF();
		if(s == 0)
			dprint("missing -I argument\n");
		else
			Ipath = s;
		break;
	case 'm':
		name = ARGF();
		if(name == 0)
			dprint("missing -m argument\n");
		break;
	}ARGEND
	symfil = 0;
	corfil = 0;
	if (argc > 0 && !alldigs(argv[0])) {
		symfil = argv[0];
		argv++;
		argc--;
	}
	if(argc==1 && alldigs(argv[0])){
		char *cpu, *p, *q;

		pid = atoi(argv[0]);
		pcsactive = 0;
		if (!symfil) {
			if(kflag){
				cpu = getenv("cputype");
				if(cpu == 0){
					cpu = "386";
					dprint("$cputype not set; assuming %s\n", cpu);
				}
				p = getenv("terminal");
				if(p==0 || (p=strchr(p, ' '))==0 || p[1]==' ' || p[1]==0){
					strcpy(b1, "/386/9pc");
					dprint("missing or bad $terminal; assuming %s\n", b1);
				}else{
					p++;
					q = strchr(p, ' ');
					if(q)
						*q = 0;
					sprint(b1, "/%s/9%s", cpu, p);
				
				}
			}else
				sprint(b1, "/proc/%s/text", argv[0]);
			symfil = b1;
		}
		sprint(b2, "/proc/%s/mem", argv[0]);
		corfil = b2;
	} else if (argc > 0) {
		fprint(2, "Usage: db [-kw] [-m machine] [-I dir] [symfile] [pid]\n");
		exits("usage");
	}
	if (!symfil)
		symfil = "8.out";
	xargc = argc;
	notify(fault);
	setsym();
	dotmap = dumbmap(-1);
	if (name && machbyname(name) == 0)
			dprint ("unknown machine %s", name);
	dprint("%s binary\n", mach->name);
	if(setjmp(env) == 0){
		if (corfil) {
			setcor();	/* could get error */
			dprint("%s\n", machdata->excep(cormap, rget));
			printpc();
		}
	}

	setjmp(env);
	if (executing)
		delbp();
	executing = FALSE;
	for (;;) {
		flushbuf();
		if (errmsg) {
			dprint(errmsg);
			printc('\n');
			errmsg = 0;
			exitflg = 0;
		}
		if (mkfault) {
			mkfault=0;
			printc('\n');
			prints(DBNAME);
		}
		clrinp();
		rdc();
		reread();
		if (eof) {
			if (infile == STDIN)
				done();
			iclose(-1, 0);
			eof = 0;
			longjmp(env, 1);
		}
		exitflg = 0;
		command(0, 0);
		reread();
		if (rdc() != '\n')
			error("newline expected");
	}
}

int
alldigs(char *s)
{
	while(*s){
		if(*s<'0' || '9'<*s)
			return 0;
		s++;
	}
	return 1;
}

void
done(void)
{
	if (pid)
		endpcs();
	exits(exitflg? "error": 0);
}

/*
 * An error occurred; save the message for later printing,
 * close open files, and reset to main command loop.
 */
void
error(char *n)
{
	errmsg = n;
	iclose(0, 1);
	oclose();
	flush();
	delbp();
	ending = 0;
	longjmp(env, 1);
}

void
errors(char *m, char *n)
{
	static char buf[128];

	sprint(buf, "%s: %s", m, n);
	error(buf);
}

/*
 * An interrupt occurred;
 * seek to the end of the current file
 * and remember that there was a fault.
 */
void
fault(void *a, char *s)
{
	USED(a);
	if(strncmp(s, "interrupt", 9) == 0){
		seek(infile, 0L, 2);
		mkfault++;
		noted(NCONT);
	}
	noted(NDFLT);
}
