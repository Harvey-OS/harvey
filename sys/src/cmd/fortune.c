#include <u.h>
#include <libc.h>
#include <bio.h>

char line[512];
char choice[512];
char index[] = "/sys/games/lib/fortunes.index";
char fortunes[] = "/sys/games/lib/fortunes";

void
main(int argc, char *argv[])
{
	int i;
	long offs;
	uchar off[4];
	int ix;
	int newindex, oldindex;
	char *p;
	Dir fbuf, ixbuf;
	Biobuf *f;

	newindex = 0;
	oldindex = 0;
	ix = offs = 0;
	if((f=Bopen(argc>1?argv[1]:fortunes, OREAD)) == 0){
		print("Misfortune!\n");
		exits("misfortune");
	}
	if(argc == 1){
		ix = open(index, OREAD);
		if(ix>=0){
			oldindex = 1;
			dirfstat(ix, &ixbuf);
			dirfstat(Bfildes(f), &fbuf);
			if(fbuf.mtime > ixbuf.mtime){
				close(ix);
				ix = create(index, OWRITE, 0666);
				if(ix >= 0){
					newindex = 1;
					oldindex = 0;
				}
			}
		}else{
			ix = create(index, OWRITE, 0666);
			if(ix >= 0)
				newindex = 1;
		}
	}
	srand(time(0)+getpid()*getpid());
	if(oldindex){
		seek(ix, nrand(ixbuf.length/sizeof(offs))*sizeof(offs), 0);
		read(ix, off, sizeof(off));
		Bseek(f, off[0]|(off[1]<<8)|(off[2]<<16)|(off[3]<<24), 0);
		p = Brdline(f, '\n');
		if(p){
			p[Blinelen(f)-1] = 0;
			strcpy(choice, p);
		}else
			strcpy(choice, "Misfortune!");
	}else{
		for(i=1;;i++){
			if(newindex)
				offs = Bseek(f, 0, 1);
			p = Brdline(f, '\n');
			if(p == 0)
				break;
			p[Blinelen(f)-1] = 0;
			strcpy(line, p);
			if(newindex){
				off[0] = offs;
				off[1] = offs>>8;
				off[2] = offs>>16;
				off[3] = offs>>24;
				write(ix, off, sizeof(off));
			}
			if(nrand(i)==0)
				strcpy(choice, line);
		}
	}
	print("%s\n", choice);
	exits(0);
}
