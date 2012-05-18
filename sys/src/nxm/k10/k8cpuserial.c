#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "io.h"

extern Dev rootdevtab;
extern Dev consdevtab;
extern Dev archdevtab;
extern Dev envdevtab;
extern Dev pipedevtab;
extern Dev procdevtab;
extern Dev mntdevtab;
extern Dev srvdevtab;
extern Dev dupdevtab;
extern Dev rtcdevtab;
extern Dev ssldevtab;
extern Dev capdevtab;
extern Dev kprofdevtab;
extern Dev segmentdevtab;
extern Dev acpidevtab;
extern Dev tubedevtab;
extern Dev zpdevtab;
extern Dev wsdevtab;
extern Dev etherdevtab;
extern Dev ipdevtab;
extern Dev pcidevtab;
extern Dev uartdevtab;
Dev* devtab[] = {
	&rootdevtab,
	&consdevtab,
	&archdevtab,
	&envdevtab,
	&pipedevtab,
	&procdevtab,
	&mntdevtab,
	&srvdevtab,
	&dupdevtab,
	&rtcdevtab,
	&ssldevtab,
	&capdevtab,
	&kprofdevtab,
	&segmentdevtab,
	&acpidevtab,
	&tubedevtab,
	&zpdevtab,
	&wsdevtab,
	&etherdevtab,
	&ipdevtab,
	&pcidevtab,
	&uartdevtab,
	nil,
};

extern uchar bootk8cpuserial_outcode[];
extern usize bootk8cpuserial_outlen;
extern uchar ____________amd64_bin_auth_factotumcode[];
extern usize ____________amd64_bin_auth_factotumlen;
extern uchar ____________amd64_bin_ip_ipconfigcode[];
extern usize ____________amd64_bin_ip_ipconfiglen;
extern uchar ___root_nvramcode[];
extern usize ___root_nvramlen;
extern void ether8169link(void);
extern void ether82557link(void);
extern void ether82563link(void);
extern void etherigbelink(void);
extern void ethermediumlink(void);
extern void loopbackmediumlink(void);
extern void netdevmediumlink(void);
void
links(void)
{
	addbootfile("boot", bootk8cpuserial_outcode, bootk8cpuserial_outlen);
	addbootfile("factotum", ____________amd64_bin_auth_factotumcode, ____________amd64_bin_auth_factotumlen);
	addbootfile("ipconfig", ____________amd64_bin_ip_ipconfigcode, ____________amd64_bin_ip_ipconfiglen);
	addbootfile("nvram", ___root_nvramcode, ___root_nvramlen);
	ether8169link();
	ether82557link();
	ether82563link();
	etherigbelink();
	ethermediumlink();
	loopbackmediumlink();
	netdevmediumlink();
}

#include "../ip/ip.h"
extern void tcpinit(Fs*);
extern void udpinit(Fs*);
extern void ipifcinit(Fs*);
extern void icmpinit(Fs*);
extern void icmp6init(Fs*);
void (*ipprotoinit[])(Fs*) = {
	tcpinit,
	udpinit,
	ipifcinit,
	icmpinit,
	icmp6init,
	nil,
};

extern PhysUart i8250physuart;
extern PhysUart pciphysuart;
PhysUart* physuart[] = {
	&i8250physuart,
	&pciphysuart,
	nil,
};

Physseg physseg[8] = {
	{	.attr	= SG_SHARED,
		.name	= "shared",
		.size	= SEGMAXPG,
	},
	{	.attr	= SG_BSS,
		.name	= "memory",
		.size	= SEGMAXPG,
	},
};
int nphysseg = 8;

char dbgflg[256];

void (*mfcinit)(void) = nil;
void (*mfcopen)(Chan*) = nil;
int (*mfcread)(Chan*, uchar*, int, vlong) = nil;
void (*mfcupdate)(Chan*, uchar*, int, vlong) = nil;
void (*mfcwrite)(Chan*, uchar*, int, vlong) = nil;

void
rdb(void)
{
	splhi();
	iprint("rdb...not installed\n");
	for(;;);
}

int cpuserver = 1;

char* conffile = "/Users/rminnich/src/nxm/sys/src/nix/k10/k8cpuserial";
ulong kerndate = KERNDATE;
