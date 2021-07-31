#include <u.h>
#include <libc.h>
#include <pool.h>

#define N(i)	((((i)*321)%1024+512)*1024)

char *p[1024];
void
movefn(void *vfrom, void *vto)
{
	int i;
	char *from, *to;

	from = vfrom;
	to = vto;
	from += 8;	/* avoid malloc padding */
	to += 8;
	print("move %p to %p\n", from, to);
	for(i=0; i<nelem(p); i++)
		if(p[i] == from)
			p[i] = to;
}

void
fill(int i)
{
	char *q;
	int j, n;

	q = p[i];
	n = N(i);
	for(j=0; j<n; j++)
		*q++ = j+i;
}

void
check(int i)
{
	char *q;
	int j, n;

	q = p[i];
	n = N(i);
	for(j=0; j<n; j++)
		if(*q++ != (char)(j+i)){
			print("lost %d p=%p j=%d n=%d j+i=%d *q=%d\n", i, p[i], j, n, j+i, *q);
			return;
		}
}

void
main(void)
{
	char xbuf[2048*1024];
	int i, j;

	mainmem->move = movefn;
	for(i=0; i<nelem(p); i++){
		p[i] = malloc(N(i));
		fill(i);
		print("i %d %d p %p\n", i, N(i), p[i]);
	}

	for(i=1; i<nelem(p)/2; i++){
		j = rand()%nelem(p);
		free(p[j]);
		p[j] = nil;
	}
	
	poolcompact(mainmem);
	for(i=0; i<nelem(p); i++){
		if(p[i])
			check(i);
	}
}
