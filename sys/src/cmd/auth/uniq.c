#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Who Who;
struct Who
{
	Who *next;
	char *line;
	char *name;
};

int cmp(void *arg1, void *arg2)
{
	Who **a = arg1, **b = arg2;

	return strcmp((*a)->name, (*b)->name);
}

void
main(int argc, char **argv)
{
	int changed, i, n;
	Biobuf *b;
	char *p, *name;
	Who *first, *last, *w, *nw, **l;

	if(argc != 2){
		fprint(2, "usage: auth/uniq file\n");
		exits(0);
	}

	last = first = 0;
	b = Bopen(argv[1], OREAD);
	if(b == 0)
		exits(0);

	n = 0;
	changed = 0;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		name = p;
		while(*p && *p != '|')
			p++;
		if(*p)
			*p++ = 0;

		for(nw = first; nw; nw = nw->next){
			if(strcmp(nw->name, name) == 0){
				free(nw->line);
				nw->line = strdup(p);
				changed = 1;
				break;
			}
		}
		if(nw)
			continue;

		w = malloc(sizeof(Who));
		if(w == 0){
			fprint(2, "auth/uniq: out of memory\n");
			exits(0);
		}
		memset(w, 0, sizeof(Who));
		w->name = strdup(name);
		w->line = strdup(p);
		if(first == 0)
			first = w;
		else
			last->next = w;
		last = w;
		n++;
	}
	Bterm(b);

	l = malloc(n*sizeof(Who*));
	for(i = 0, nw = first; nw; nw = nw->next, i++)
		l[i] = nw;
	qsort(l, n, sizeof(Who*), cmp);

	if(!changed)
		exits(0);

	b = Bopen(argv[1], OWRITE);
	if(b == 0){
		fprint(2, "auth/uniq: can't open %s\n", argv[1]);
		exits(0);
	}
	for(i = 0; i < n; i++)
		Bprint(b, "%s|%s\n", l[i]->name, l[i]->line);
	Bterm(b);
}
