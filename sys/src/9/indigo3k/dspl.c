#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Seg	Seg;

struct Seg
{
	int	type;		/* P, X, Y */
	long	base;		/* from loader output */
	long	hpcbase;	/* address in #A/dspmem */
	int	size;		/* allocated */
	int	nwords;		/* used */
	ulong *	data;
};

Seg*	newseg(int, ulong);
void	putseg(Seg*, ulong);
void	putsegn(Seg*, ulong*, int);
int	segcmp(void*, void*);

int	debug;
char *	infile;
Biobuf *in;
Biobuf *out;

int	Cflag;
int	cflag;
int	lflag;
int	vflag;
Seg	*segs;
int	nsegs;

void
main(int argc, char **argv)
{
	char *l, *p;
	int i, k, type = 0, lineno = 0;
	ulong base;
	Seg *sp = 0, *sq = 0;
	int cfd, mfd;

	cflag = 16;
	ARGBEGIN{
	case 'c':	/* combine segments */
		cflag = strtol(ARGF(), 0, 0);
		break;
	case 'C':	/* write C data structure */
		++Cflag;
		break;
	case 'l':	/* load */
		++lflag;
		break;
	case 'v':	/* verbose */
		++vflag;
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	infile = (argc > 0) ? argv[0] : "/fd/0";
	in = Bopen(infile, OREAD);
	if(in == 0){
		perror(infile);
		exits("in");
	}
	out = Bopen("/fd/1", OWRITE);
	while(l = Brdline(in, '\n')){	/* assign = */
		++lineno;
		l[BLINELEN(in)-1] = 0;
		if(l[0] == 0)
			continue;
		if(strncmp(l, "_START ", 7) == 0)
			continue;
		if(strncmp(l, "_END ", 5) == 0)
			continue;
		if(strncmp(l, "_DATA ", 6) == 0){
			p = l+6;
			p += strspn(p, " ");
			type = *p++;
			base = strtoul(p, 0, 16);
			if(type == 'L'){
				sp = newseg('X', base);
				sq = newseg('Y', base);
			}else
				sp = newseg(type, base);
			if(debug)
				fprint(2, "%s %d: %c 0x%.4ux\n",
					infile, lineno, sp->type, sp->base);
			continue;
		}
		if(l[0] == '_' || sp == 0){
			fprint(2, "%s %d: %s\n", infile, lineno, l);
			exits("fmt");
		}
		i = 0;
		for(p=strtok(l, " "); p; p=strtok(0, " ")){
			putseg(i==0?sp:sq, strtoul(p, 0, 16));
			if(type == 'L')
				i = 1-i;
		}
	}

	if(cflag >= 0)
		qsort(segs, nsegs, sizeof(Seg), segcmp);
	if(cflag > 0)for(i=0; i<nsegs-1; i++){
		sp = &segs[i];
		sq = sp+1;
		if(sp->type != sq->type)
			continue;
		k = sq->base - (sp->base+sp->nwords);
		if(k >= cflag)
			continue;
		if(k < 0){
			fprint(2, "overlap in %c\n", sp->type);
			continue;
		}
		while(--k >= 0)
			putseg(sp, 0);
		putsegn(sp, sq->data, sq->nwords);
		free(sq->data);
		--nsegs;
		memmove(sq, sq+1, (nsegs-i)*sizeof(Seg));
		--i;
	}
	if(vflag)for(i=0; i<nsegs; i++){
		sp = &segs[i];
		print("0x%.4ux 0x%.4ux %c: 0x%.4ux 0x%.4ux %6d\n",
			sp->hpcbase, sp->hpcbase+sp->nwords,
			sp->type, sp->base, sp->base+sp->nwords, sp->nwords);
	}
	if(Cflag)for(i=0; i<nsegs; i++){
		sp = &segs[i];
		Bprint(out, "\t0x%.2ux,0x%.2ux, 0x%.2ux,0x%.2ux,\n",
			sp->hpcbase>>8, sp->hpcbase&0xff,
			sp->nwords>>8, sp->nwords&0xff);
		for(k=0; k<sp->nwords; k++){
			Bprint(out, "%c0x%.2ux,0x%.2ux,0x%.2ux,",
				k%4?' ': '\t', sp->data[k]>>16,
				(sp->data[k]>>8)&0xff, sp->data[k]&0xff);
			if(k%4 == 3)
				Bprint(out, "\n");
		}
		if(k%4)
			Bprint(out, "\n");
		if(i == nsegs-1)
			Bprint(out, "\t0, 0, 0, 0\n");
	}
	if(lflag){
		cfd = open("#A/dspctl", OWRITE);
		mfd = open("#A/dspmem", OWRITE);
		if(mfd < 0 || cfd < 0){
			fprint(2, "can't open #A: %r\n");
			exits("open_#A");
		}
		fprint(cfd, "reset\n");
		for(i=0; i<nsegs; i++){
			sp = &segs[i];
			seek(mfd, sp->hpcbase*sizeof(long), 0);
			write(mfd, sp->data, sp->nwords*sizeof(long));
		}
		fprint(cfd, "start\n");
	}
	exits(0);
}

Seg *
newseg(int type, ulong base)
{
	Seg *sp;

	segs = realloc(segs, (nsegs+1)*sizeof(Seg));
	sp = &segs[nsegs++];
	sp->type = type;
	sp->base = base;
	sp->hpcbase = base;
	if(sp->type != 'Y')
		sp->hpcbase ^= 0x4000;
	sp->hpcbase -= 0x8000;
	sp->size = 0;
	sp->nwords = 0;
	sp->data = 0;
	return sp;
}

void
incrseg(Seg *sp, int n)
{
	int k;

	k = n - sp->size;

	if(k < 256)
		k = 256;
	sp->size += k;
	sp->data = realloc(sp->data, sp->size*sizeof(ulong));
	memset(&sp->data[sp->nwords], 0, k*sizeof(ulong));
}

void
putseg(Seg *sp, ulong data)
{
	if(sp->nwords >= sp->size)
		incrseg(sp, sp->nwords+1);
	sp->data[sp->nwords++] = data;
}

void
putsegn(Seg *sp, ulong *datap, int n)
{
	if(sp->nwords+n > sp->size)
		incrseg(sp, sp->nwords+n);
	memmove(&sp->data[sp->nwords], datap, n*sizeof(ulong));
	sp->nwords += n;
}

int
segcmp(Seg *sp, Seg *sq)
{
	return sp->hpcbase - sq->hpcbase;
}
