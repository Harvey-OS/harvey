#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev archdevtab;
extern Dev envdevtab;
extern Dev pipedevtab;
extern Dev procdevtab;
extern Dev mntdevtab;
extern Dev srvdevtab;
extern Dev dupdevtab;
extern Dev ssldevtab;
extern Dev bridgedevtab;
extern Dev etherdevtab;
extern Dev ipdevtab;
extern Dev sddevtab;
extern Dev floppydevtab;
extern Dev ns16552devtab;
Dev* devtab[]={
	&rootdevtab,
	&consdevtab,
	&archdevtab,
	&envdevtab,
	&pipedevtab,
	&procdevtab,
	&mntdevtab,
	&srvdevtab,
	&dupdevtab,
	&ssldevtab,
	&bridgedevtab,
	&etherdevtab,
	&ipdevtab,
	&sddevtab,
	&floppydevtab,
	&ns16552devtab,
	nil,
};

extern uchar ipconfigcode[];
extern ulong ipconfiglen;
extern void ether2114xlink(void);
extern void ethermediumlink(void);
void links(void){
	addrootfile("ipconfig", ipconfigcode, ipconfiglen);
	ether2114xlink();
	ethermediumlink();
}

extern PCArch arch164;
PCArch* knownarch[] = {
	&arch164,
	nil,
};

#include "sd.h"
extern SDifc sdataifc;
extern SDifc sd53c8xxifc;
SDifc* sdifc[] = {
	&sdataifc,
	&sd53c8xxifc,
	nil,
};

#include "../ip/ip.h"
extern void ilinit(Fs*);
extern void tcpinit(Fs*);
extern void udpinit(Fs*);
extern void rudpinit(Fs*);
extern void ipifcinit(Fs*);
extern void icmpinit(Fs*);
extern void greinit(Fs*);
extern void ipmuxinit(Fs*);
void (*ipprotoinit[])(Fs*) = {
	ilinit,
	tcpinit,
	udpinit,
	rudpinit,
	ipifcinit,
	icmpinit,
	greinit,
	ipmuxinit,
	nil,
};

	int cpuserver = 1;
char* conffile = "apccpu";
ulong kerndate = KERNDATE;
