#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	"fcall.h"

Dev	devtab[]={
	{ consreset, consinit, consattach, consclone, conswalk, consstat, consopen, conscreate,
	  consclose, consread, conswrite, consremove, conswstat, },
	{ envreset, envinit, envattach, envclone, envwalk, envstat, envopen, envcreate,
	  envclose, envread, envwrite, envremove, envwstat, },
	{ mntreset, mntinit, mntattach, mntclone, mntwalk, mntstat, mntopen, mntcreate,
	  mntclose, mntread, mntwrite, mntremove, mntwstat, },
	{ duartreset, duartinit, duartattach, duartclone, duartwalk, duartstat, duartopen, duartcreate,
	  duartclose, duartread, duartwrite, duartremove, duartwstat, },
};
char *devchar="ceMt";
void streaminit0(void){
}
	long cfslen = 0; ulong cfscode[1];
	long kfslen = 0; ulong kfscode[1];
	void kproftimer(ulong pc) {USED(pc);}
	void consdebug(void){}
	void lanceintr(void){}
	cpuserver = 1;
char	*conffile = "powerminor";
ulong	kerndate = KERNDATE;
