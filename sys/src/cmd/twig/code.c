#include "common.h"
#include "code.h"
#define BLOCKSIZE	100

Code *cdfreep = NULL;

Code *
CodeGetBlock(void)
{
	static int count = 0;
	static Code *blockp = NULL;
	Code *cp;

	if(cdfreep){
		cp = cdfreep;
		cdfreep = cdfreep->prev;
	}else{
		if(!count){
			blockp = Malloc(BLOCKSIZE*sizeof(Code));
			count = BLOCKSIZE;
		}
		cp = blockp++;
		count--;
	}
	cp->prev = NULL;
	cp->firstFree = cp->segment;
	return cp;
}

void
CodeFreeBlock(Code *cd)
{
	if(cd){
		cd->prev = cdfreep;
		cdfreep = cd;
	}
}

Code *
CodeStoreChar(Code *cd, char c)
{
	if(cd->firstFree - cd->segment >= CSEGSIZE) {
		Code *ncd = CodeGetBlock();
		ncd->prev = cd;
		cd = ncd;
	}
	*cd->firstFree++ = c;
	return cd;
}

Code *
CodeStoreString(Code *cd, char *s)
{
	while(*s != '\0')
		cd = CodeStoreChar(cd, *s++);
	return cd;
}

Code *
CodeAppend(Code *cd1, Code *cd2)
{
	if(!cd2)
		return cd1;
	cd2->prev = CodeAppend(cd1, cd2->prev);
	return cd2;
}

void
CodeWrite(Biobuf *b, Code *cd)
{
	if(!cd)
		return;
	CodeWrite(b, cd->prev);
	Bwrite(b, cd->segment, cd->firstFree-cd->segment);
}

Code *
CodeMarkLine(Code *cd, int lineno)
{
	char buf[100];

	sprint(buf,"\n#line %d \"%s\"\n", lineno, inFileName);
	return CodeStoreString(cd, buf);
}
