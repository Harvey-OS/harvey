/* Copyright 1990, AT&T Bell Labs */
#include "fsort.h"

int	mflag = 0;
int	cflag = 0;
int	keyed = 0;

extern	void	readin(void);
extern	void	dumptotemp(Rec*, int);
extern	void	sealstack(Rec *p);

FILE*	input;
char*	dummyargv[] = { "sort", "-", 0 };
char*	oname = "-";
char*	tname[] = { "/tmp"/*substitutable*/, "/tmp", "/tmp", 0 };

#define squeezargs(n,i,nsq ) 		\
	for(i=n; i<argc-nsq; i++)	\
		argv[i] = argv[i+nsq];	\
	argc -= nsq

void
main(int argc, char **argv)
{
	int n, i, k;

	setvbuf(stdout, 0, _IOLBF, 0);

	for(n=1; n<argc; ) {
		if(argv[n][0] == '-')
			switch(argv[n][1]) {
			case 'o':
				if(argv[n][2]) {
					oname = argv[n]+2;
					squeezargs(n, i, 1);
				} else
				if(n < argc-1) {
					oname = argv[n+1];
					squeezargs(n, i, 2);
				} else
					fatal("incomplete -o","",0);
				continue;
			case 'T':
				if(argv[n][2]) {
					tname[0] = argv[n]+2;
					squeezargs(n, i, 1);
				} else
				if(n < argc-1) {
					tname[0] = argv[n+1];
					squeezargs(n, i, 2);
				} else
					fatal("incomplete -T","",0);
				continue;
			case 'y':
				optiony(argv[n]+2);
			case 'z':
				squeezargs(n, i, 1);
				continue;
			}
		n++;
	}
 	for(n=1; n<argc; ) {
		k = fieldarg(argv[n], n+1<argc? argv[n+1]: 0);
		squeezargs(n, i, k);
		if(k == 0)
			n++;
	}
	fieldwrapup();
	if(argc <= 1) {
		argc = 2;
		argv = dummyargv;
	}
	tabinit();
	mergeinit();

	if(cflag) {
		if(argc > 2)
			fatal("-c takes just one file", "", 0);
		check(argv[1]);
		exits(0);
	} else if(mflag) {
		merge(argc, argv);
		exits(0);
	}
	while(--argc >= 1) {
		input = fileopen(*++argv, "r");
		readin();
		fileclose(input, "");
	}
	if(stack->head) {
		Rec *r = succ(stack->tail);

		r->dlen = 0;		/* keep dumptotemp sane */
		if(stack->head->next)
			sort(stack, 0);
		if(nextfile > 0) {
			dumptotemp(r, 0);
			tabfree();
			mergetemp();
		} else {
			FILE *f;
			f = fileopen(oname, "w");
			printout(stack->head, f, oname);
			fileclose(f, oname);
		}
	} else
	if(strcmp(oname,"-") != 0) 	/* empty input */
		fileclose(fileopen(oname, "w"), oname);

	exits(0);
}

void
readin(void)
{
	Rec *p = stack->tail;
	Rec *r = p? succ(p): buffer;

	for(;;) {
		switch(getline(r, bufmax, input)) {
		case -1:
			sealstack(p);
			return;
		case -2:
			sealstack(p);
			dumptotemp(r, 1);
			p = 0;
			r = buffer;
		}
		if(!keyed)
			r->klen = 0;
		else for(;;) {		/* loops back at most once */
			r->klen = fieldcode(data(r), key(r), r->dlen, bufmax);
			if(r->klen >= 0)
				break;
			r->klen = 0;
			sealstack(p);
			dumptotemp(r, 0);
			p = 0;
			r = buffer;
		}
		r->next = 0;
		if(p)
			p->next = r;
		p = r;
		r = succ(r);
	}
}

void
sealstack(Rec *p)
{
	if(p == 0)
		return;
	p->next = 0;
	if(stack->head == 0)
		stack->head = buffer;
	stack->tail = p;
}

void
printout(Rec *r, FILE *f, char *name)
{
	uchar *dp;
	int n;

	for(; r; r=r->next) {
		for(n=r->dlen, dp=data(r); --n>=0; )
			if(putc(*dp++, f) == EOF)
				fatal("error writing", name, 0);
		if(putc('\n', f) == EOF)
			fatal("error writing", name, 0);
	}
}

void
dumptotemp(Rec *r, int partial)
{
	Rec *p;
	char *tempfile = filename(nextfile++);
	FILE *temp = fileopen(tempfile,"w");
	int n;

	stack->tail->next = 0;		/* for good measure */
	sort(stack, 0);
	printout(stack->head, temp, tempfile);
	fileclose(temp, tempfile);
	free(tempfile);

	memmove((char*)buffer, (char*)r, sizeof(*r)+r->dlen);
	r = buffer;
	r->klen = 0;
	r->next = 0;
	if(partial) {
		p = succ(r);
		if(getline(p, bufmax, input) == -2)
			fatal("monster record", "", 0);
		n = p->dlen;
		memmove((char*)data(r)+r->dlen, (char*)data(p), n);
		r->dlen += n;
	}
	stack->head = stack->tail = 0;
}
