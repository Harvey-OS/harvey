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
	{ mntreset, mntinit, mntattach, mntclone, mntwalk, mntstat, mntopen, mntcreate,
	  mntclose, mntread, mntwrite, mntremove, mntwstat, },
	{ bitreset, bitinit, bitattach, bitclone, bitwalk, bitstat, bitopen, bitcreate,
	  bitclose, bitread, bitwrite, bitremove, bitwstat, },
	{ uartreset, uartinit, uartattach, uartclone, uartwalk, uartstat, uartopen, uartcreate,
	  uartclose, uartread, uartwrite, uartremove, uartwstat, },
	{ pipereset, pipeinit, pipeattach, pipeclone, pipewalk, pipestat, pipeopen, pipecreate,
	  pipeclose, piperead, pipewrite, piperemove, pipewstat, },
	{ floppyreset, floppyinit, floppyattach, floppyclone, floppywalk, floppystat, floppyopen, floppycreate,
	  floppyclose, floppyread, floppywrite, floppyremove, floppywstat, },
	{ atareset, atainit, ataattach, ataclone, atawalk, atastat, ataopen, atacreate,
	  ataclose, ataread, atawrite, ataremove, atawstat, },
	{ srvreset, srvinit, srvattach, srvclone, srvwalk, srvstat, srvopen, srvcreate,
	  srvclose, srvread, srvwrite, srvremove, srvwstat, },
	{ dupreset, dupinit, dupattach, dupclone, dupwalk, dupstat, dupopen, dupcreate,
	  dupclose, dupread, dupwrite, dupremove, dupwstat, },
	{ procreset, procinit, procattach, procclone, procwalk, procstat, procopen, proccreate,
	  procclose, procread, procwrite, procremove, procwstat, },
	{ rtcreset, rtcinit, rtcattach, rtcclone, rtcwalk, rtcstat, rtcopen, rtccreate,
	  rtcclose, rtcread, rtcwrite, rtcremove, rtcwstat, },
	{ etherreset, etherinit, etherattach, etherclone, etherwalk, etherstat, etheropen, ethercreate,
	  etherclose, etherread, etherwrite, etherremove, etherwstat, },
	{ ipreset, ipinit, ipattach, ipclone, ipwalk, ipstat, ipopen, ipcreate,
	  ipclose, ipread, ipwrite, ipremove, ipwstat, },
	{ arpreset, arpinit, arpattach, arpclone, arpwalk, arpstat, arpopen, arpcreate,
	  arpclose, arpread, arpwrite, arpremove, arpwstat, },
	{ iproutereset, iprouteinit, iprouteattach, iprouteclone, iproutewalk, iproutestat, iprouteopen, iproutecreate,
	  iprouteclose, iprouteread, iproutewrite, iprouteremove, iproutewstat, },
	{ vgareset, vgainit, vgaattach, vgaclone, vgawalk, vgastat, vgaopen, vgacreate,
	  vgaclose, vgaread, vgawrite, vgaremove, vgawstat, },
	{ dkreset, dkinit, dkattach, dkclone, dkwalk, dkstat, dkopen, dkcreate,
	  dkclose, dkread, dkwrite, dkremove, dkwstat, },
	{ inconreset, inconinit, inconattach, inconclone, inconwalk, inconstat, inconopen, inconcreate,
	  inconclose, inconread, inconwrite, inconremove, inconwstat, },
	{ scsireset, scsiinit, scsiattach, scsiclone, scsiwalk, scsistat, scsiopen, scsicreate,
	  scsiclose, scsiread, scsiwrite, scsiremove, scsiwstat, },
	{ wrenreset, wreninit, wrenattach, wrenclone, wrenwalk, wrenstat, wrenopen, wrencreate,
	  wrenclose, wrenread, wrenwrite, wrenremove, wrenwstat, },
	{ cdromreset, cdrominit, cdromattach, cdromclone, cdromwalk, cdromstat, cdromopen, cdromcreate,
	  cdromclose, cdromread, cdromwrite, cdromremove, cdromwstat, },
};
Rune *devchar=L"/ceMbt|fHsdprlIaPvkiSwR";
extern Qinfo perminfo;
extern void	stiplink(void);
extern void	stfcalllink(void);
extern void	stasynclink(void);
extern void	sturplink(void);
extern void	ether509link(void);
extern void	ether8003link(void);
extern void	ether2000link(void);
extern void	vgaclgd542xlink(void);
extern void	vgaet4000link(void);
extern void	vgas3link(void);
extern void	vgabt485link(void);
extern void	vgatvp3020link(void);
extern uchar	fscode[];
extern ulong	fslen;
void streaminit(void){
	newqinfo(&perminfo);
	stiplink();
	stfcalllink();
	stasynclink();
	sturplink();
	ether509link();
	ether8003link();
	ether2000link();
	vgaclgd542xlink();
	vgaet4000link();
	vgas3link();
	vgabt485link();
	vgatvp3020link();
	addrootfile("fs", fscode, fslen);
}
	int cpuserver = 0;
	void consdebug(void){}
	int incondev = 0;
char	*conffile = "pccd";
ulong	kerndate = KERNDATE;
