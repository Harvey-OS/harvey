#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
usage(void)
{
	fprint(2, "usage: fmtindex config\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	Config conf;
	Index *ix;
	ArenaPart *ap;
	Arena **arenas;
	AMap *amap;
	u64int addr;
	char *file;
	u32int i, j, n, narenas;
	int add;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	add = 0;
	ARGBEGIN{
	case 'a':
		add = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

	file = argv[0];

	if(!runConfig(file, &conf))
		fatal("can't intialization config %s: %R", file);
	if(conf.index == nil)
		fatal("no index specified in %s", file);
	if(!nameOk(conf.index))
		fatal("illegal index name %s", conf.index);

	narenas = 0;
	for(i = 0; i < conf.naparts; i++){
		ap = conf.aparts[i];
		narenas += ap->narenas;
	}

	if(add){
		ix = initIndex(conf.index, conf.sects, conf.nsects);
		if(ix == nil)
			fatal("can't initialize index %s: %R", conf.index);
	}else{
		ix = newIndex(conf.index, conf.sects, conf.nsects);
		if(ix == nil)
			fatal("can't create new index %s: %R", conf.index);

		n = 0;
		for(i = 0; i < ix->nsects; i++)
			n += ix->sects[i]->blocks;

		if(ix->div < 100)
			fatal("index divisor too coarse: use bigger block size");

		fprint(2, "using %ud buckets of %ud; div=%d\n", ix->buckets, n, ix->div);
	}
	amap = MKNZ(AMap, narenas);
	arenas = MKNZ(Arena*, narenas);

	addr = IndexBase;
	n = 0;
	for(i = 0; i < conf.naparts; i++){
		ap = conf.aparts[i];
		for(j = 0; j < ap->narenas; j++){
			if(n >= narenas)
				fatal("too few slots in index's arena set");

			arenas[n] = ap->arenas[j];
			if(n < ix->narenas){
				if(arenas[n] != ix->arenas[n])
					fatal("mismatched arenas %s and %s at slot %d\n",
						arenas[n]->name, ix->arenas[n]->name, n);
				amap[n] = ix->amap[n];
				if(amap[n].start != addr)
					fatal("mis-located arena %s in index %s\n", arenas[n]->name, ix->name);
				addr = amap[n].stop;
			}else{
				amap[n].start = addr;
				addr += ap->arenas[j]->size;
				amap[n].stop = addr;
				nameCp(amap[n].name, ap->arenas[j]->name);
				fprint(2, "add arena %s at [%lld,%lld)\n",
					amap[n].name, amap[n].start, amap[n].stop);
			}

			n++;
		}
	}
	fprint(2, "configured index=%s with arenas=%d and storage=%lld\n",
		ix->name, n, addr - IndexBase);

	ix->amap = amap;
	ix->arenas = arenas;
	ix->narenas = narenas;

	if(!wbIndex(ix))
		fprint(2, "can't write back arena partition header for %s: %R\n", file);

	exits(0);
	return 0;	/* shut up stupid compiler */
}
