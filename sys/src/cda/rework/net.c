#include	"rework.h"

static
lcmp(const void *a, const void *b)
{
	return *(long *)a - *(long *)b;
}

Net::Net(FILE *fp, Wire **carry, int ins)
{
	Wire *w, ww[NPTS];
	long points[NPTS];
	int n, i, sing;
	long singles[NPTS];

	n = 0;
	if(*carry)
		ww[n++] = **carry;
	else {
		w = readwire(fp);
		if(w == 0){
			fprintf(stderr, "no wires in file!!\n");
			exit(1);
		}
		ww[n++] = *w;
	}
	while(w = readwire(fp)){
		if(strcmp(w->f3, ww[0].f3))
			break;
		ww[n++] = *w;
	}
	nalloc = nwires = n;
	wires = new Wire[nalloc];
	memcpy((char *)wires, (char *)ww, sizeof(Wire)*nalloc);
	*carry = w;
	name = wires[0].f3;
	for(n = 0, w = wires; n < nwires; n++, w++){
		points[2*n] = PT(w->x1, w->y1);
		points[2*n+1] = PT(w->x2, w->y2);
	}
	qsort((char *)points, 2*nwires, sizeof(long), lcmp);
	pts = new long[2*nwires];
	for(i = 2*nwires, sing = 0, n = nwires; --i >= 0; ){
		pts[n--] = points[i];
		if((points[i-1] == points[i]) && (i > 0))
			i--;
		else {
			singles[sing++] = points[i];
			if(!eflag && (sing > 2))
				break;
		}
	}
	if(!eflag && (sing > 2)){
		fprintf(stderr, "net %s has at least %d ends: %d/%d %d/%d %d/%d\n", name,
			sing, XY(singles[0]), XY(singles[1]), XY(singles[2]));
fprintf(stderr, "nwire=%d nalloc=%d carry=%s\n", nwires, nalloc, (*carry)->f3);
for(i = 0; i < 2*nwires; i++) fprintf(stderr, "[%d] %d\n", i, points[i]);
		exit(1);
	}
	if(ins){
		w = wires;
		pins.install(PT(w->x1, w->y1), this);
		for(n = 0; n < nwires; n++, w++)
			pins.install(PT(w->x2, w->y2), this);
	}
	done = 0;
	next = 0;
}

Net *
readnets(FILE *fp, int ins)
{
	Wire *w;
	Net *n, *root;

	w = 0;
	root = n = new Net(fp, &w, ins);
	while(w){
		n->next = new Net(fp, &w, ins);
		n = n->next;
	}
	return(root);
}

int
Net::in(long l)
{
	register i;

	for(i = 0; i < nwires+1; i++)
		if(l == pts[i])
			return(1);
	return(0);
}

void
Net::pr(FILE *fp, char *newname)
{
	register i;

	for(i = 0; i < nwires; i++)
		wires[i].pr(fp, newname);
}

void
Net::rm()
{
	if(verbose)
		vpr("rm old");
	else {
		if(DEBUG(name))
			fprintf(stderr, "unwrap old net %s\n", name);
		pr(UN, name);
	}
}
