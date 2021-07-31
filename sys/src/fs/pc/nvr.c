#include "all.h"
#include "mem.h"
#include "io.h"
#include "ureg.h"

#include "dosfs.h"

static Dosfile file;
static int opened;
char nvrfile[128] = "plan9.nvr";

int
nvread(int offset, void *a, int n)
{
	int r, s;

	if(opened == 0 && dosopen(&dos, nvrfile, &file) == 0)
		panic("can't open %s\n", nvrfile);
	opened = 1;

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

	if(opened == 0 && dosopen(&dos, nvrfile, &file) == 0)
		panic("can't open %s\n", nvrfile);
	opened = 1;

	s = spllo();
	file.offset = offset;
	r = doswrite(&file, a, n);
	splx(s);
	return r;
}
