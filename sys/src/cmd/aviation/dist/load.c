#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "obj.h"

FILE	*fin;
FILE	*fout;
Obj	o;
File	fo;
int	nitems;

extern	void	xload(void);

void
main(int argc, char *argv[])
{
	int i;

	fout = fopen(oname, "a");
	if(fout == NULL) {
		print("cannot open %s\n", oname);
		exits("open");
	}
	if(sizeof(fo) != sizeof(fo.name)+sizeof(fo.type)+
	   sizeof(fo.flag)+sizeof(fo.lat)+sizeof(fo.lng)+sizeof(fo.freq)) {
		print("not sum\n");
		exits("sum");
	}
	for(i=1; i<argc; i++) {
		nitems = 0;
		fin = fopen(argv[i], "r");
		if(fin == NULL) {
			print("cannot open %s\n", argv[i]);
			exits("open");
		}
		xload();
		fclose(fin);
		print("%s %d items\n", argv[i], nitems);
	}
	fclose(fout);
	exits(0);
}

void
xload(void)
{
	int int1, int2, c, i;


loop:
	for(i=0; i<NNAME; i++)
		o.name[i] = 0;
	for(i=0;; i++) {
		c = fgetc(fin);
		while(c == ' ' || c == '\n')
			c = fgetc(fin);
		if(c == EOF)
			goto out;
		if(c == ',')
			break;
		if(i < NNAME)
			o.name[i] = c;
	}
	c = fscanf(fin, "%d,%d,", &int1, &int2);
	if(c == EOF) {
		print("scan failed1 %s\n", o.name);
		goto loop;
	}
	o.type = int1;
	o.flag = int2;
	c = fscanf(fin, "%f,%f,%f,", &o.freq, &o.lat, &o.lng);
	if(c == EOF) {
		print("scan failed2 %s\n", o.name);
		goto loop;
	}
	o.lat *= RADIAN;
	o.lng *= RADIAN;

	memcpy(fo.name, o.name, sizeof(fo.name));
	fo.type = o.type;
	fo.flag = o.flag;
	sprint(fo.lat, "%.6f", o.lat);
	if(strlen(fo.lat)+1 > sizeof(fo.lat)) {
		print("overflow on lat %s %s\n", fo.name, fo.lat);
		goto bad;
	}
	sprint(fo.lng, "%.6f", o.lng);
	if(strlen(fo.lng)+1 > sizeof(fo.lng)) {
		print("overflow on lng %s %s\n", fo.name, fo.lng);
		goto bad;
	}
	sprint(fo.freq, "%.2f", o.freq);
	if(strlen(fo.freq)+1 > sizeof(fo.freq)) {
		print("overflow on freq %s %s\n", fo.name, fo.freq);
		goto bad;
	}
	fwrite(&fo, sizeof(fo), 1, fout);
	nitems++;
	goto loop;
out:
	return;

bad:
	exits("error");
}
