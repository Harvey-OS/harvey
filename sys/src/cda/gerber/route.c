#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>

static void pr_mst(Signal *);
static void pr_seq(Signal *);
static void hack(Signal *);
Board b;

void
main(int argc, char **argv)
{
	int n;
	extern Signal *maxsig;
	extern void tsp(Signal *);
	extern void mst(Signal *);
	extern int optind;
	void (*fn)(Signal *) = tsp;
	void (*prfn)(Signal *) = pr_seq;

	while((n = getopt(argc, argv, "tm")) != -1)
		switch(n)
		{
		case 't':	fn = tsp; prfn = pr_seq; break;
		case 'm':	fn = mst; prfn = pr_mst; break;
		case '?':	fprint(2, "Usage: %s [-mt] [files]\n", argv0);
				exits("usage");
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", *argv, n);
			exit(1);
		}
	fizzplane(&b);
	for(n = 0; n < b.nplanes; n++)
		if(strcmp(b.planes[n].sig, "analog_route") == 0)
			break;
	if(n >= b.nplanes){
		fprint(2, "no plane ``analog_route''\n");
		exits("no plane");
	}
	if(n = fizzplace()){
		fprint(2, "%d chips unplaced\n", n);
		exits("unplaced");
	}
	if(n = fizzprewrap()){
		fprint(2, "%d prewrap errors\n", n);
		exits("prewrap errors");
	}
	symtraverse(S_SIGNAL, hack);
	symtraverse(S_SIGNAL, netlen);
	if(maxsig && ((maxsig->type & VSIG) != VSIG) && (maxsig->n >= MAXNET)){
		fprint(2, "net %s is too big (%d>=%d)\n", maxsig->name, maxsig->n, MAXNET);
		exits("bignet");
	}
	symtraverse(S_SIGNAL, fn);
	symtraverse(S_SIGNAL, prfn);
	exit(0);
}

static void
hack(Signal *s)
{
	if(s->type == VSIG)
		s->type = NORMSIG;
}

static void
pr_mst(Signal *s)
{
	int i, j, k;
	short cur[2*MAXNET+1];
	short pt[2*MAXNET+1];
	short map[MAXNET];
	int lo, hi;
	char drill;

	if(s->type != NORMSIG)
		return;
	k = 0;
	for(i = 0; i < s->n;){
		lo = hi = MAXNET;
		cur[lo] = s->pins[i].used;
		map[cur[lo]] = i++;
		cur[++hi] = s->pins[i].used;
		map[cur[hi]] = i++;
		for(; i < s->n; i++){
			j = s->pins[i].used;
			if(j == cur[lo]){
				cur[--lo] = s->pins[++i].used;
				map[cur[lo]] = i;
			} else if(j == cur[hi]){
				cur[++hi] = s->pins[++i].used;
				map[cur[hi]] = i;
			} else
				break;
		}
		for(; lo < hi; lo++)
			pt[k++] = map[cur[lo]];
		pt[k++] = -map[cur[lo]];
	}
	print("Route {\n\tname %s\n\talg hand\n\tlayout %d{\n", s->name, k);
	drill = 'Z';
	for(j = 0; j < k; j++){
		i = (pt[j]<0)? -pt[j]:pt[j];
		if(drill == 'Z')
			drill = 'A';
		else
			drill = ' ';
		if(pt[j] < 0)
			drill = 'Z';
		print("\t\t%d\t%d/%d %c\t%% %s.%d\n", j+1, s->pins[i].p.x,
			s->pins[i].p.y, drill, s->coords[i].chip, s->coords[i].pin);
	}
	print("\t}\n}\n");
}

static void
pr_seq(Signal *s)
{
	int i;

	if(s->type != NORMSIG)
		return;
	print("Route {\n\tname %s\n\talg hand\n\tlayout %d{\n", s->name, s->n);
	for(i = 0; i < s->n; i++){
		print("\t\t%d\t%d/%d%s\t%% %s.%d\n", i+1, s->pins[i].p.x,
			s->pins[i].p.y, (i==0)?" A":"", s->coords[i].chip, s->coords[i].pin);
	}
	print("\t}\n}\n");
}
