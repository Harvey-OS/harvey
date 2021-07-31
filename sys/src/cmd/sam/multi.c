#include "sam.h"

List	file;
ushort	tag;

File *
newfile(void)
{
	File *f;

	inslist(&file, 0, (long)(f = Fopen()));
	f->tag = tag++;
	if(downloaded)
		outTs(Hnewname, f->tag);
	/* already sorted; file name is "" */
	return f;
}

int
whichmenu(File *f)
{
	int i;

	for(i=0; i<file.nused; i++)
		if(file.filepptr[i]==f)
			return i;
	return -1;
}

void
delfile(File *f)
{
	int w = whichmenu(f);

	if(w < 0)	/* e.g. x/./D */
		return;
	if(downloaded)
		outTs(Hdelname, f->tag);
	dellist(&file, w);
	Fclose(f);
}

void
sortname(File *f)
{
	int i, cmp, w;
	int dupwarned;

	w = whichmenu(f);
	dupwarned = FALSE;
	dellist(&file, w);
	if(f == cmd)
		i = 0;
	else for(i=0; i<file.nused; i++){
		cmp = Strcmp(&f->name, &file.filepptr[i]->name);
		if(cmp==0 && !dupwarned){
			dupwarned = TRUE;
			warn_S(Wdupname, &f->name);
		}else if(cmp<0 && (i>0 || cmd==0))
			break;
	}
	inslist(&file, i, (long)f);
	if(downloaded)
		outTsS(Hmovname, f->tag, &f->name);
}

void
state(File *f, int cleandirty)
{
	if(f == cmd)
		return;
	if(downloaded && whichmenu(f)>=0){	/* else flist or menu */
		if(f->state==Dirty && cleandirty!=Dirty)
			outTs(Hclean, f->tag);
		else if(f->state!=Dirty && cleandirty==Dirty)
			outTs(Hdirty, f->tag);
	}
	f->state = cleandirty;
}

File *
lookfile(String *s)
{
	int i;

	for(i=0; i<file.nused; i++)
		if(Strcmp(&file.filepptr[i]->name, s) == 0)
			return file.filepptr[i];
	return 0;
}
