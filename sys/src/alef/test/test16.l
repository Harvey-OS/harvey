#include <alef.h>
#include <bio.h>

chan(int) a0, a1, a2, a3, a4, a5, a6, a7, a8, a9;

void
produce(chan(int) j, int c)
{
	Biobuf b;
	int x, fd;
	byte buf[50];

	sprint(buf, "xx%c", c);
	fd = create(buf, OWRITE, 0666);
	b.inits(fd, OWRITE, buf, sizeof(buf));
	for(;;) {
		x =<- j;
		b.print("test16.l:       %c -> %d\n", c, x);
		if (x >= 9)
			break;
	}
}

void
main(void)
{
	int i;

	alloc a0, a1, a2, a3, a4, a5, a6, a7, a8, a9;

	proc produce(a0, '0');
	proc produce(a1, '1');
	proc produce(a2, '2');
	proc produce(a3, '3');
	proc produce(a4, '4');
	proc produce(a5, '5');
	proc produce(a6, '6');
	proc produce(a7, '7');
	proc produce(a8, '8');
	proc produce(a9, '9');

	for(i = 0; i < 10; i++)
		par {
			a0<-=i;
			a1<-=i;
			a2<-=i;
			a3<-=i;
			a4<-=i;
			a5<-=i;
			a6<-=i;
			a7<-=i;
			a8<-=i;
			a9<-=i;
		}
	print("output in files xx[0-9]\n");
}
