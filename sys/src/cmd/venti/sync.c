#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static	int	verbose;
void
usage(void)
{
	fprint(2, "usage: sync [-fv] [-B blockcachesize] config\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	u32int bcmem;
	int fix;

	fix = 0;
	bcmem = 0;
	ARGBEGIN{
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	case 'f':
		fix++;
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(!fix)
		readonly = 1;

	if(argc != 1)
		usage();

	vtAttach();
	if(!initVenti(argv[0]))
		fatal("can't init venti: %R");

	if(bcmem < maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16))
		bcmem = maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16);
	fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initDCache(bcmem);

	if(verbose)
		printIndex(2, mainIndex);
	if(!syncIndex(mainIndex, fix))
		fatal("failed to sync index=%s: %R\n", mainIndex->name);

	exits(0);
	return 0;	/* shut up stupid compiler */
}
