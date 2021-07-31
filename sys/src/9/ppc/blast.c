#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev envdevtab;
extern Dev flashdevtab;
extern Dev pipedevtab;
extern Dev procdevtab;
extern Dev mntdevtab;
extern Dev srvdevtab;
extern Dev dupdevtab;
extern Dev ssldevtab;
extern Dev capdevtab;
extern Dev kprofdevtab;
extern Dev uartdevtab;
extern Dev irqdevtab;
extern Dev realtimedevtab;
extern Dev etherdevtab;
extern Dev ipdevtab;
Dev* devtab[]={
	&rootdevtab,
	&consdevtab,
	&envdevtab,
	&flashdevtab,
	&pipedevtab,
	&procdevtab,
	&mntdevtab,
	&srvdevtab,
	&dupdevtab,
	&ssldevtab,
	&capdevtab,
	&kprofdevtab,
	&uartdevtab,
	&irqdevtab,
	&realtimedevtab,
	&etherdevtab,
	&ipdevtab,
	nil,
};

extern void etherfcclink(void);
extern void ethermediumlink(void);
extern void netdevmediumlink(void);
void links(void){
	bootlinks();

	etherfcclink();
	ethermediumlink();
	netdevmediumlink();
}

extern PhysUart smcphysuart;
PhysUart* physuart[] = {
	&smcphysuart,
	nil,
};

#include "../ip/ip.h"
extern void ilinit(Fs*);
extern void tcpinit(Fs*);
extern void udpinit(Fs*);
extern void ipifcinit(Fs*);
extern void icmpinit(Fs*);
extern void icmp6init(Fs*);
void (*ipprotoinit[])(Fs*) = {
	ilinit,
	tcpinit,
	udpinit,
	ipifcinit,
	icmpinit,
	icmp6init,
	nil,
};

	int cpuserver = 1;
char* conffile = "blast";
ulong kerndate = KERNDATE;
