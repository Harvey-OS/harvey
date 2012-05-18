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
extern Dev pmcdevtab;
extern Dev segmentdevtab;
extern Dev etherdevtab;
extern Dev ipdevtab;
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
	&pmcdevtab,
	&segmentdevtab,
	&etherdevtab,
	&ipdevtab,
	&uartdevtab,
	nil,
};

extern uchar boot_fscode[];
extern usize boot_fslen;
extern uchar _amd64_bin_rccode[];
extern usize _amd64_bin_rclen;
extern uchar _rc_lib_rcmaincode[];
extern usize _rc_lib_rcmainlen;
extern uchar _amd64_bin_echocode[];
extern usize _amd64_bin_echolen;
extern uchar _amd64_bin_datecode[];
extern usize _amd64_bin_datelen;
extern uchar _amd64_bin_execaccode[];
extern usize _amd64_bin_execaclen;
extern uchar _amd64_bin_lscode[];
extern usize _amd64_bin_lslen;
extern uchar _amd64_bin_pscode[];
extern usize _amd64_bin_pslen;
extern uchar _amd64_bin_bindcode[];
extern usize _amd64_bin_bindlen;
extern uchar _amd64_bin_catcode[];
extern usize _amd64_bin_catlen;
extern uchar _amd64_bin_auth_factotumcode[];
extern usize _amd64_bin_auth_factotumlen;
extern uchar _amd64_bin_ip_ipconfigcode[];
extern usize _amd64_bin_ip_ipconfiglen;
extern uchar ___root_bigcode[];
extern usize ___root_biglen;
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
	addbootfile("boot", boot_fscode, boot_fslen);
	addbootfile("rc", _amd64_bin_rccode, _amd64_bin_rclen);
	addbootfile("rcmain", _rc_lib_rcmaincode, _rc_lib_rcmainlen);
	addbootfile("echo", _amd64_bin_echocode, _amd64_bin_echolen);
	addbootfile("date", _amd64_bin_datecode, _amd64_bin_datelen);
	addbootfile("execac", _amd64_bin_execaccode, _amd64_bin_execaclen);
	addbootfile("ls", _amd64_bin_lscode, _amd64_bin_lslen);
	addbootfile("ps", _amd64_bin_pscode, _amd64_bin_pslen);
	addbootfile("bind", _amd64_bin_bindcode, _amd64_bin_bindlen);
	addbootfile("cat", _amd64_bin_catcode, _amd64_bin_catlen);
	addbootfile("factotum", _amd64_bin_auth_factotumcode, _amd64_bin_auth_factotumlen);
	addbootfile("ipconfig", _amd64_bin_ip_ipconfigcode, _amd64_bin_ip_ipconfiglen);
	addbootfile("big", ___root_bigcode, ___root_biglen);
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

extern void cinit(void);
extern void copen(Chan*);
extern int cread(Chan*, uchar*, int, vlong);
extern void cupdate(Chan*, uchar*, int, vlong);
extern void cwrite(Chan*, uchar*, int, vlong);

void (*mfcinit)(void) = cinit;
void (*mfcopen)(Chan*) = copen;
int (*mfcread)(Chan*, uchar*, int, vlong) = cread;
void (*mfcupdate)(Chan*, uchar*, int, vlong) = cupdate;
void (*mfcwrite)(Chan*, uchar*, int, vlong) = cwrite;

void
rdb(void)
{
	splhi();
	iprint("rdb...not installed\n");
	for(;;);
}

int cpuserver = 1;

char* conffile = "/Users/rminnich/src/nxm/sys/src/nix/k10/k8cpufs";
ulong kerndate = KERNDATE;
