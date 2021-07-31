#include "geom.h"
#include "thing.h"
#include "text.h"
#include "wire.h"
#include "conn.h"
#include "box.h"
#include "master.h"

Vector *masters;

void process(FILE *inf)
{
	Master *m = new Master(new String("toplevel"));
	m->get(inf);
	fclose(inf);
	m->process();
	delete m;
	masters->clear();
}

int kflag;	/* kahrs (proximity) flag */
int mflag;	/* macro flag - treat macros more like boxes */
char *filename = "stdin.g";
char *stem = "";
extern "C" void exit(int);
main(int argc, char *argv[])
{
	int i;
	char *s;
	FILE *inf;
	masters = new Vector(32);
	if (argc == 1)
		process(stdin);
	else
		for (i = 1; i < argc == 1; i++) {
			if (argv[i][0] == '-' && argv[i][1] == 'k')
				kflag = 1;
			else if (argv[i][0] == '-' && argv[i][1] == 'm') {
				mflag = 1;
				stem = argv[++i];
			}
			else if (argc == 1 || *argv[i] == '-')
				process(stdin);
			else
				if ((inf = fopen(filename=argv[i],"r")) == 0)
					fprintf(stderr,"can't open %s\n",argv[i]);
				else {
					if (!mflag) {
						for (s = filename; *s != 0 && *s != '.'; s++)
							;
						*s = 0;
						stem = filename;
					}
					process(inf);
				}
		}
	exit(0);
}

strlen(char *s)
{
	int i;
	for (i = 0; *s++; i++)
		;
	return i;
}

Rectangle Rectangle::mbb(Point p)
{
	Rectangle r;
	r.o.x = min(o.x,p.x);
	r.o.y = min(o.y,p.y);
	r.c.x = max(c.x,p.x);
	r.c.y = max(c.y,p.y);
	return r;
}

Rectangle Rectangle::mbb(Rectangle r)
{
	r.o.x = min(o.x,r.o.x);
	r.o.y = min(o.y,r.o.y);
	r.c.x = max(c.x,r.c.x);
	r.c.y = max(c.y,r.c.y);
	return r;
}
