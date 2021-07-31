#include "all.h"
#include "mem.h"
#include "io.h"
#include "ureg.h"

#include "dosfs.h"

static Dosfile file;
static int opened;
char nvrfile[128] = "plan9.nvr";

static void
nvopen(void)
{
	int s;
	Dosfile *fp;

	if(opened)
		return;
	opened = 1;
	s = spllo();
	fp = dosopen(&dos, nvrfile, &file);
	splx(s);
	if(fp == 0)
		panic("can't open %s\n", nvrfile);
}

int
nvread(int offset, void *a, int n)
{
	int r, s;

	nvopen();

	s = spllo();
	file.offset = offset;
	r = dosread(&file, a, n);
	splx(s);
	return r;
}

int
nvwrite(int offset, void *a, int n)
{
	int r, s;

	nvopen();

	s = spllo();
	file.offset = offset;
	r = doswrite(&file, a, n);
	splx(s);
	return r;
}
