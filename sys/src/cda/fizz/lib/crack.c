#include <u.h>
#include <libc.h>
#include <stdio.h>
#ifdef PLAN9
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include	<cda/fizz.h>

int f_iline;
FILE *ifp;
char *f_ifname;
int f_nerrs;
Board *f_b;
char * comment;
static char linebuf[BUFSIZ];

extern void f_board(char *), f_chip(char *), f_package(char *);
extern void f_pinholes(char *), f_positions(char *), f_net(char *);
extern void f_type(char *), f_vsig(char *), f_route(char *);
extern void f_wires(char *);

static Keymap goo[] = {
	"Board", f_board, 0,
	"Chip", f_chip, 1,
	"Net", f_net, 2,
	"Package", f_package, 3,
	"Pinholes", f_pinholes, 4,
	"Positions", f_positions, 5,
	"Route", f_route, 6,
	"Type", f_type, 7,
	"Wires", f_wires, 8,
	"Vsig", f_vsig, 9,
	0
};

int
f_crack(char *file, Board *b)
{
	char *s;
	int i;
	void (*fn)(char *);

	if((ifp = fopen(file, "r")) == 0){
		perror(file);
		return(1);
	}
	f_iline = 0;
	f_ifname = file;
	f_nerrs = 0;
	f_b = b;

	while(s = f_inline()){
		if((i = f_keymap(goo, &s)) >= 0) {
			fn = goo[i].fn;
			(*fn)(s);
		}
		else
			fprint(2,"unrecognized input: %s\n",s);
	}
	fclose(ifp);
	return(f_nerrs);
}

void
f_init(Board *b)
{
	int i;

	b->name = 0;
	b->align[0].x = -1;
	for(i = 0; i < 6; i++){
		b->v[i].signo = i;
		b->v[i].name = 0;
		b->v[i].npins = 0;
	}
	b->ne.x = b->se.x = -MAXCOORD/2;
	b->nw.x = b->sw.x = MAXCOORD/2;
	b->se.y = b->sw.y = MAXCOORD/2;
	b->ne.y = b->nw.y = -MAXCOORD/2;
	b->prect.min.x = b->prect.min.y = MAXCOORD;
	b->prect.max.x = b->prect.max.y = 0;
	b->pinholes = 0;
	b->planes = 0;
	b->nplanes = 0;
	b->keepouts = 0;
	b->nkeepouts = 0;
	for(i = 0; i < MAXLAYER; i++)
		b->layer[i] = 0;
	b->layers = 0;
	b->datums[0].drill = b->datums[1].drill = b->datums[2].drill = 0;
	b->ndrillsz = 0;
	b->drillsz = (Drillsz *) 0;
	b->ndrills = 0;
	b->drills = (Pin *) 0;
	b->xyid  = (char *) 0;
	b->xydef = (char *) 0;
}

char *
f_inline(void)
{
	register char *s, *ss;
	int n;

again:
	comment = "";
	f_iline++;
	if(s = fgets(linebuf, sizeof(linebuf), ifp)){
		BLANK(s);
		if(ss = strchr(s, '%')){
			comment = ss + 1;
			while((--ss >= s) && ISBL(*ss))
				;
			ss[1] = 0;
		}
		if(*s == 0 || *s == '\n')
			goto again;
		n = strlen(s)-1;
		if (s[n] == '\n')
			s[n] = 0;
	}
	return(s);
}

char *
f_tabline(void)
{
	register char *s, *ss;
	int n;

again:
	f_iline++;
	if(s = fgets(linebuf, sizeof(linebuf), ifp)){
		while(*s == '\t')
			s++;
		if(ss = strchr(s, '%')){
			while((--ss >= s) && ISBL(*ss))
				;
			ss[1] = 0;
		}
		if(*s == 0 || *s == '\n')
			goto again;
		n = strlen(s)-1;
		if (s[n] == '\n')
			s[n] = 0;
	}
	return(s);
}


void
f_major(char *fmt, va_list args)
{
	char buf[500], *out;

	out = buf + sprint(buf, "%s: %d: ", f_ifname, f_iline);
	out = doprint(out, &buf[4095], fmt, &args);
	*out++ = '\n';
	write(2, buf, (long)(out-buf));
	f_nerrs++;
}

void
f_minor(char *fmt, va_list args)
{
	char buf[500], *out;

	out = buf + sprint(buf, "%s: %d: ", f_ifname, f_iline);
	out = doprint(out, &buf[4095], fmt, &args);
	*out++ = '\n';
	write(2, buf, (long)(out-buf));
	f_nerrs++;
}

char *
f_malloc(long n)
{
	char *s;

	if(s = lmalloc(n))
		memset(s, 0, n);
	return(s);
}
