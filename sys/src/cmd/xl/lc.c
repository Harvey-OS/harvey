#include <u.h>
#include <libc.h>
#include <a.out.h>
#include <bio.h>

void	check(char*);
void	tab(uchar*, int, char*, char*);
void	error(char*, ...);

void
main(int argc, char *argv[])
{
	int i;

	ARGBEGIN{
	}ARGEND

	for(i = 0; i < argc; i++)
		check(argv[i]);
	exits(0);
}

void
check(char *name)
{
	Exec e;
	uchar *pc;
	int fd, n;

	fd = open(name, OREAD);
	if(fd < 0)
		return;
	if(read(fd, &e, sizeof e) != sizeof e){
		close(fd);
		return;
	}
	switch(e.magic){
	case A_MAGIC:
	case Z_MAGIC:
	case I_MAGIC:
	case J_MAGIC:
	case K_MAGIC:
	case V_MAGIC:
	case X_MAGIC:
		break;
	default:
		close(fd);
		return;
	}
	n = e.pcsz;
	if(!n){
		print("no pc-line table for %s\n", name);
		close(fd);
		return;
	}
	pc = malloc(n+1);
	if(!pc)
		error("out of memory %d", n);
	pc[n] = '\0';
	seek(fd, e.text + e.data + e.syms + e.spsz, 0);
	if(read(fd, pc, n) != n)
		error("can't read pc line for %s", name);
	tab(pc, n, name, "pc-line");
	free(pc);
	n = e.spsz;
	if(!n){
		close(fd);
		return;
	}
	pc = malloc(n+1);
	if(!pc)
		error("out of memory %d", n);
	pc[n] = '\0';
	seek(fd, e.text + e.data + e.syms, 0);
	if(read(fd, pc, n) != n)
		error("can't read pc line for %s", name);
	tab(pc, n, name, "pc-sp");
	free(pc);
	close(fd);
}


void
tab(uchar *pc, int n, char *name, char *t)
{
	int i, c, m, mt, md, d, x;

	m = mt = md = 0;
	x = 0;
	for(i = 0; i < n; i++){
		c = pc[i];
		if(c == 0){
			i += 4;
			continue;
		}
		if(c == 255){
			m++;
			d = 0;
			for(i++; pc[i] == 255; i++)
				d++;
			if(pc[i] < 129)
				x++;
			if(d > md)
				md = d;
			mt += d;
			m += d;
			if(pc[i] + d*3 >= 253)
				x++;
		}
	}
	if(m)
		print("%s: %s: %d 255's %d stacked %d max depth %d bigger\n",
			name, t, m, mt, md, x);
}

void
error(char *s, ...)
{
	char buf[2*ERRLEN];

	doprint(buf, buf+sizeof(buf), s, (&s+1));
	fprint(2, "rx: %s\n", buf);
	exits(buf);
}
