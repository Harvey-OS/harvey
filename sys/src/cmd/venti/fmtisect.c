#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
usage(void)
{
	fprint(2, "usage: fmtisect [-Z] [-b blocksize] [-d directorySize] name file\n");
	exits(0);
}

int
main(int argc, char *argv[])
{
	ISect *is;
	Part *part;
	char *file, *name;
	int blockSize, setSize, zero;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	blockSize = 8 * 1024;
	setSize = 64 * 1024;
	zero = 1;
	ARGBEGIN{
	case 'b':
		blockSize = unittoull(ARGF());
		if(blockSize == ~0)
			usage();
		if(blockSize > MaxDiskBlock){
			fprint(2, "block size too large, max %d\n", MaxDiskBlock);
			exits("usage");
		}
		break;
	case 'Z':
		zero = 0;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 2)
		usage();

	name = argv[0];
	file = argv[1];

	if(!nameOk(name))
		fatal("illegal name %s", name);

	part = initPart(file, 0);
	if(part == nil)
		fatal("can't open partition %s: %r", file);

	if(zero)
		zeroPart(part, blockSize);

	fprint(2, "configuring index section %s with space for index config bytes=%d\n", name, setSize);
	is = newISect(part, name, blockSize, setSize);
	if(is == nil)
		fatal("can't initialize new index: %R");

	if(!wbISect(is))
		fprint(2, "can't write back index section header for %s: %R\n", file);

	exits(0);
	return 0;	/* shut up stupid compiler */
}
