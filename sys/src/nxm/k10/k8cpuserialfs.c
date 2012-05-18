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

extern uchar boot_fscode[];
extern usize boot_fslen;
extern uchar _NXM_amd64_bin_rccode[];
extern usize _NXM_amd64_bin_rclen;
extern uchar _NXM_rc_lib_rcmaincode[];
extern usize _NXM_rc_lib_rcmainlen;
extern uchar _NXM_amd64_bin_echocode[];
extern usize _NXM_amd64_bin_echolen;
extern uchar _NXM_amd64_bin_datecode[];
extern usize _NXM_amd64_bin_datelen;
extern uchar _NXM_amd64_bin_execaccode[];
extern usize _NXM_amd64_bin_execaclen;
extern uchar _NXM_amd64_bin_lscode[];
extern usize _NXM_amd64_bin_lslen;
extern uchar _NXM_amd64_bin_pscode[];
extern usize _NXM_amd64_bin_pslen;
extern uchar _NXM_amd64_bin_bindcode[];
extern usize _NXM_amd64_bin_bindlen;
extern uchar _NXM_amd64_bin_catcode[];
extern usize _NXM_amd64_bin_catlen;
extern uchar _NXM_amd64_bin_ratracecode[];
extern usize _NXM_amd64_bin_ratracelen;
extern uchar _NXM_amd64_bin_sleepcode[];
extern usize _NXM_amd64_bin_sleeplen;
extern uchar _NXM_amd64_bin_auth_factotumcode[];
extern usize _NXM_amd64_bin_auth_factotumlen;
extern uchar _NXM_amd64_bin_ip_ipconfigcode[];
extern usize _NXM_amd64_bin_ip_ipconfiglen;
extern uchar ___root_nvramcode[];
extern usize ___root_nvramlen;
extern uchar ___root_6_testkmsegcode[];
extern usize ___root_6_testkmseglen;
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
	addbootfile("boot", boot_fscode, boot_fslen);
	addbootfile("rc", _NXM_amd64_bin_rccode, _NXM_amd64_bin_rclen);
	addbootfile("rcmain", _NXM_rc_lib_rcmaincode, _NXM_rc_lib_rcmainlen);
	addbootfile("echo", _NXM_amd64_bin_echocode, _NXM_amd64_bin_echolen);
	addbootfile("date", _NXM_amd64_bin_datecode, _NXM_amd64_bin_datelen);
	addbootfile("execac", _NXM_amd64_bin_execaccode, _NXM_amd64_bin_execaclen);
	addbootfile("ls", _NXM_amd64_bin_lscode, _NXM_amd64_bin_lslen);
	addbootfile("ps", _NXM_amd64_bin_pscode, _NXM_amd64_bin_pslen);
	addbootfile("bind", _NXM_amd64_bin_bindcode, _NXM_amd64_bin_bindlen);
	addbootfile("cat", _NXM_amd64_bin_catcode, _NXM_amd64_bin_catlen);
	addbootfile("ratrace", _NXM_amd64_bin_ratracecode, _NXM_amd64_bin_ratracelen);
	addbootfile("sleep", _NXM_amd64_bin_sleepcode, _NXM_amd64_bin_sleeplen);
	addbootfile("factotum", _NXM_amd64_bin_auth_factotumcode, _NXM_amd64_bin_auth_factotumlen);
	addbootfile("ipconfig", _NXM_amd64_bin_ip_ipconfigcode, _NXM_amd64_bin_ip_ipconfiglen);
	addbootfile("nvram", ___root_nvramcode, ___root_nvramlen);
	addbootfile("testkmseg", ___root_6_testkmsegcode, ___root_6_testkmseglen);
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

char* conffile = "/Users/rminnich/src/nxm/sys/src/nix/k10/k8cpuserialfs";
ulong kerndate = KERNDATE;
