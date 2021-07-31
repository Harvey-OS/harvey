#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	"fcall.h"

Dev	devtab[]={
	{ rootreset, rootinit, rootattach, rootclone, rootwalk, rootstat, rootopen, rootcreate,
	  rootclose, rootread, rootwrite, rootremove, rootwstat, },
	{ consreset, consinit, consattach, consclone, conswalk, consstat, consopen, conscreate,
	  consclose, consread, conswrite, consremove, conswstat, },
	{ envreset, envinit, envattach, envclone, envwalk, envstat, envopen, envcreate,
	  envclose, envread, envwrite, envremove, envwstat, },
	{ pipereset, pipeinit, pipeattach, pipeclone, pipewalk, pipestat, pipeopen, pipecreate,
	  pipeclose, piperead, pipewrite, piperemove, pipewstat, },
	{ procreset, procinit, procattach, procclone, procwalk, procstat, procopen, proccreate,
	  procclose, procread, procwrite, procremove, procwstat, },
	{ srvreset, srvinit, srvattach, srvclone, srvwalk, srvstat, srvopen, srvcreate,
	  srvclose, srvread, srvwrite, srvremove, srvwstat, },
	{ mntreset, mntinit, mntattach, mntclone, mntwalk, mntstat, mntopen, mntcreate,
	  mntclose, mntread, mntwrite, mntremove, mntwstat, },
	{ inconreset, inconinit, inconattach, inconclone, inconwalk, inconstat, inconopen, inconcreate,
	  inconclose, inconread, inconwrite, inconremove, inconwstat, },
	{ dkreset, dkinit, dkattach, dkclone, dkwalk, dkstat, dkopen, dkcreate,
	  dkclose, dkread, dkwrite, dkremove, dkwstat, },
	{ dupreset, dupinit, dupattach, dupclone, dupwalk, dupstat, dupopen, dupcreate,
	  dupclose, dupread, dupwrite, dupremove, dupwstat, },
	{ bitreset, bitinit, bitattach, bitclone, bitwalk, bitstat, bitopen, bitcreate,
	  bitclose, bitread, bitwrite, bitremove, bitwstat, },
	{ portreset, portinit, portattach, portclone, portwalk, portstat, portopen, portcreate,
	  portclose, portread, portwrite, portremove, portwstat, },
	{ scsireset, scsiinit, scsiattach, scsiclone, scsiwalk, scsistat, scsiopen, scsicreate,
	  scsiclose, scsiread, scsiwrite, scsiremove, scsiwstat, },
	{ wrenreset, wreninit, wrenattach, wrenclone, wrenwalk, wrenstat, wrenopen, wrencreate,
	  wrenclose, wrenread, wrenwrite, wrenremove, wrenwstat, },
	{ duartreset, duartinit, duartattach, duartclone, duartwalk, duartstat, duartopen, duartcreate,
	  duartclose, duartread, duartwrite, duartremove, duartwstat, },
};
char devchar[]="/ce|psMikdbxSwt";
extern Qinfo	urpinfo;
extern Qinfo	asyncinfo;
extern Qinfo	fcallinfo;
void streaminit0(void){
	newqinfo(&urpinfo);
	newqinfo(&asyncinfo);
	newqinfo(&fcallinfo);
}
	#include "cfs.h"
	#include "kfs.h"
	void kproftimer(ulong pc){}
