#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "httpd.h"
#include "xml.h"

void xmlArena(Hio *hout, Arena *s, char *tag, int indent){
	xmlIndent(hout, indent);
	hprint(hout, "<%s", tag);
	xmlAName(hout, s->name, "name");
	xmlU32int(hout, s->version, "version");
	xmlAName(hout, s->part->name, "partition");
	xmlU32int(hout, s->blockSize, "blockSize");
	xmlU64int(hout, s->base, "start");
	xmlU64int(hout, s->base+2*s->blockSize, "stop");
	xmlU32int(hout, s->ctime, "created");
	xmlU32int(hout, s->wtime, "modified");
	xmlSealed(hout, s->sealed, "sealed");
	xmlScore(hout, s->score, "score");
	xmlU32int(hout, s->clumps, "clumps");
	xmlU32int(hout, s->cclumps, "compressedClumps");
	xmlU64int(hout, s->uncsize, "data");
	xmlU64int(hout, s->used - s->clumps * ClumpSize, "compressedData");
	xmlU64int(hout, s->used + s->clumps * ClumpInfoSize, "storage");
	hprint(hout, "/>\n");
}

void xmlIndex(Hio *hout, Index *s, char *tag, int indent){
	int i;
	xmlIndent(hout, indent);
	hprint(hout, "<%s", tag);
	xmlAName(hout, s->name, "name");
	xmlU32int(hout, s->version, "version");
	xmlU32int(hout, s->blockSize, "blockSize");
	xmlU32int(hout, s->tabSize, "tabSize");
	xmlU32int(hout, s->buckets, "buckets");
	xmlU32int(hout, s->div, "buckDiv");
	hprint(hout, ">\n");
	xmlIndent(hout, indent + 1);
	hprint(hout, "<sects>\n");
	for(i = 0; i < s->nsects; i++)
		xmlAmap(hout, &s->smap[i], "sect", indent + 2);
	xmlIndent(hout, indent + 1);
	hprint(hout, "</sects>\n");
	xmlIndent(hout, indent + 1);
	hprint(hout, "<amaps>\n");
	for(i = 0; i < s->narenas; i++)
		xmlAmap(hout, &s->amap[i], "amap", indent + 2);
	xmlIndent(hout, indent + 1);
	hprint(hout, "</amaps>\n");
	xmlIndent(hout, indent + 1);
	hprint(hout, "<arenas>\n");
	for(i = 0; i < s->narenas; i++)
		xmlArena(hout, s->arenas[i], "arena", indent + 2);
	xmlIndent(hout, indent + 1);
	hprint(hout, "</arenas>\n");
	xmlIndent(hout, indent);
	hprint(hout, "</%s>\n", tag);
}

void xmlAmap(Hio *hout, AMap *s, char *tag, int indent){
	xmlIndent(hout, indent);
	hprint(hout, "<%s", tag);
	xmlAName(hout, s->name, "name");
	xmlU64int(hout, s->start, "start");
	xmlU64int(hout, s->stop, "stop");
	hprint(hout, "/>\n");
}

