#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
/*
 * Just a wrapper for all the initialization routines
 */
int plinit(int ldepth){
	if(!pl_drawinit(ldepth)) return 0;
	return 1;
}
