#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

Dev	devtab[]={
	{ rootreset, rootinit, rootattach, rootclone, rootwalk, rootstat, rootopen, rootcreate,
	  rootclose, rootread, rootwrite, rootremove, rootwstat, },
	{ consreset, consinit, consattach, consclone, conswalk, consstat, consopen, conscreate,
	  consclose, consread, conswrite, consremove, conswstat, },
	{ envreset, envinit, envattach, envclone, envwalk, envstat, envopen, envcreate,
	  envclose, envread, envwrite, envremove, envwstat, },
	{ procreset, procinit, procattach, procclone, procwalk, procstat, procopen, proccreate,
	  procclose, procread, procwrite, procremove, procwstat, },
	{ lancereset, lanceinit, lanceattach, lanceclone, lancewalk, lancestat, lanceopen, lancecreate,
	  lanceclose, lanceread, lancewrite, lanceremove, lancewstat, },
	{ mntreset, mntinit, mntattach, mntclone, mntwalk, mntstat, mntopen, mntcreate,
	  mntclose, mntread, mntwrite, mntremove, mntwstat, },
	{ hsreset, hsinit, hsattach, hsclone, hswalk, hsstat, hsopen, hscreate,
	  hsclose, hsread, hswrite, hsremove, hswstat, },
	{ inconreset, inconinit, inconattach, inconclone, inconwalk, inconstat, inconopen, inconcreate,
	  inconclose, inconread, inconwrite, inconremove, inconwstat, },
	{ breakerreset, breakerinit, breakerattach, breakerclone, breakerwalk, breakerstat, breakeropen, breakercreate,
	  breakerclose, breakerread, breakerwrite, breakerremove, breakerwstat, },
	{ dkreset, dkinit, dkattach, dkclone, dkwalk, dkstat, dkopen, dkcreate,
	  dkclose, dkread, dkwrite, dkremove, dkwstat, },
	{ iproutereset, iprouteinit, iprouteattach, iprouteclone, iproutewalk, iproutestat, iprouteopen, iproutecreate,
	  iprouteclose, iprouteread, iproutewrite, iprouteremove, iproutewstat, },
	{ arpreset, arpinit, arpattach, arpclone, arpwalk, arpstat, arpopen, arpcreate,
	  arpclose, arpread, arpwrite, arpremove, arpwstat, },
	{ ipreset, ipinit, ipattach, ipclone, ipwalk, ipstat, ipopen, ipcreate,
	  ipclose, ipread, ipwrite, ipremove, ipwstat, },
	{ sccreset, sccinit, sccattach, sccclone, sccwalk, sccstat, sccopen, scccreate,
	  sccclose, sccread, sccwrite, sccremove, sccwstat, },
	{ bootreset, bootinit, bootattach, bootclone, bootwalk, bootstat, bootopen, bootcreate,
	  bootclose, bootread, bootwrite, bootremove, bootwstat, },
	{ bitreset, bitinit, bitattach, bitclone, bitwalk, bitstat, bitopen, bitcreate,
	  bitclose, bitread, bitwrite, bitremove, bitwstat, },
};
char *devchar="/ceplMhikPaItBb";
extern Qinfo	urpinfo;
extern Qinfo	ipinfo;
extern Qinfo	fcallinfo;
void streaminit(void){
	newqinfo(&urpinfo);
	newqinfo(&ipinfo);
	newqinfo(&fcallinfo);
}
	int cpuserver = 1;
	void consdebug(void){}
	void kproftimer(ulong pc){ USED(pc); }
	long kfslen = 0; ulong kfscode[1];
	long cfslen = 0; ulong cfscode[1];
char	*conffile = "magnumbrk";
ulong	kerndate = KERNDATE;
