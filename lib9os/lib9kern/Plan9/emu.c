#include "dat.h"
#include "fns.h"
#include "error.h"
#include "interp.h"


#include "emu.root.h"

ulong ndevs = 16;

extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev envdevtab;
extern Dev mntdevtab;
extern Dev procdevtab;
extern Dev fsdevtab;
extern Dev dynlddevtab;
extern Dev srv9devtab;
Dev* devtab[]={
	&rootdevtab,
	&consdevtab,
	&envdevtab,
	&mntdevtab,
	&procdevtab,
	&fsdevtab,
	&dynlddevtab,
	&srv9devtab,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
};

void links(void){
}

void modinit(void){
}

char* conffile = "emu";
ulong kerndate = KERNDATE;
