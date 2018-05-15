#include <u.h>
#include "fns.h"
#include "efi.h"

typedef UINT16	EFI_PXE_BASE_CODE_UDP_PORT;

typedef struct {
	UINT8		Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct {
	UINT8		Addr[16];
} EFI_IPv6_ADDRESS;

typedef union {
	UINT32			Addr[4];
	EFI_IPv4_ADDRESS	v4;
	EFI_IPv6_ADDRESS	v6;
} EFI_IP_ADDRESS;

typedef struct {
	UINT8		Addr[32];
} EFI_MAC_ADDRESS;

typedef struct {
	UINT8		BootpOpcode;
	UINT8		BootpHwType;
	UINT8		BootpHwAddrLen;
	UINT8		BootpGateHops;
	UINT32		BootpIdent;
	UINT16		BootpSeconds;
	UINT16		BootpFlags;
	UINT8		BootpCiAddr[4];
	UINT8		BootpYiAddr[4];
	UINT8		BootpSiAddr[4];
	UINT8		BootpGiAddr[4];
	UINT8		BootpHwAddr[16];
	UINT8		BootpSrvName[64];
	UINT8		BootpBootFile[128];
	UINT32		DhcpMagik;
	UINT8		DhcpOptions[56];		
} EFI_PXE_BASE_CODE_DHCPV4_PACKET;

typedef struct {
	BOOLEAN		Started;
	BOOLEAN		Ipv6Available;
	BOOLEAN		Ipv6Supported;
	BOOLEAN		UsingIpv6;
	BOOLEAN		BisSupported;
	BOOLEAN		BisDetected;
	BOOLEAN		AutoArp;
	BOOLEAN		SendGUID;
	BOOLEAN		DhcpDiscoverValid;
	BOOLEAN		DhcpAckReceived;
	BOOLEAN		ProxyOfferReceived;
	BOOLEAN		PxeDiscoverValid;
	BOOLEAN		PxeReplyReceived;
	BOOLEAN		PxeBisReplyReceived;
	BOOLEAN		IcmpErrorReceived;
	BOOLEAN		TftpErrorReceived;
	BOOLEAN		MakeCallbacks;

	UINT8		TTL;
	UINT8		ToS;

	UINT8		Reserved;

	UINT8		StationIp[16];
	UINT8		SubnetMask[16];

	UINT8		DhcpDiscover[1472];
	UINT8		DhcpAck[1472];
	UINT8		ProxyOffer[1472];
	UINT8		PxeDiscover[1472];
	UINT8		PxeReply[1472];
	UINT8		PxeBisReply[1472];

} EFI_PXE_BASE_CODE_MODE;

typedef struct {
	UINT64		Revision;
	void		*Start;
	void		*Stop;
	void		*Dhcp;
	void		*Discover;
	void		*Mtftp;
	void		*UdpWrite;
	void		*UdpRead;
	void		*SetIpFilter;
	void		*Arp;
	void		*SetParameters;
	void		*SetStationIp;
	void		*SetPackets;
	EFI_PXE_BASE_CODE_MODE	*Mode;
} EFI_PXE_BASE_CODE_PROTOCOL;


enum {
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Tftp_OACK	= 6,

	TftpPort	= 69,

	Segsize		= 512,
};

static
EFI_GUID EFI_PXE_BASE_CODE_PROTOCOL_GUID = {
	0x03C4E603, 0xAC28, 0x11D3,
	0x9A, 0x2D, 0x00, 0x90,
	0x27, 0x3F, 0xC1, 0x4D,
};

static
EFI_PXE_BASE_CODE_PROTOCOL *pxe;

static uchar mymac[6];
static uchar myip[16];
static uchar serverip[16];

typedef struct Tftp Tftp;
struct Tftp
{
	EFI_IP_ADDRESS sip;
	EFI_IP_ADDRESS dip;

	EFI_PXE_BASE_CODE_UDP_PORT sport;
	EFI_PXE_BASE_CODE_UDP_PORT dport;

	char *rp;
	char *ep;

	int seq;
	int eof;
	
	char pkt[2+2+Segsize];
	char nul;
};

static void
puts(void *x, ushort v)
{
	uchar *p = x;

	p[1] = (v>>8) & 0xFF;
	p[0] = v & 0xFF;
}

static ushort
gets(void *x)
{
	uchar *p = x;

	return p[1]<<8 | p[0];
}

static void
hnputs(void *x, ushort v)
{
	uchar *p = x;

	p[0] = (v>>8) & 0xFF;
	p[1] = v & 0xFF;
}

static ushort
nhgets(void *x)
{
	uchar *p = x;

	return p[0]<<8 | p[1];
}

enum {
	ANY_SRC_IP	= 0x0001,
	ANY_SRC_PORT	= 0x0002,
	ANY_DEST_IP	= 0x0004,
	ANY_DEST_PORT	= 0x0008,
	USE_FILTER	= 0x0010,
	MAY_FRAGMENT	= 0x0020,
};

static int
udpread(EFI_IP_ADDRESS *sip, EFI_IP_ADDRESS *dip, 
	EFI_PXE_BASE_CODE_UDP_PORT *sport, 
	EFI_PXE_BASE_CODE_UDP_PORT dport, 
	int *len, void *data)
{
	UINTN size;

	size = *len;
	if(eficall(pxe->UdpRead, pxe, (UINTN)ANY_SRC_PORT, dip, &dport, sip, sport, nil, nil, &size, data))
		return -1;

	*len = size;
	return 0;
}

static int
udpwrite(EFI_IP_ADDRESS *dip,
	EFI_PXE_BASE_CODE_UDP_PORT sport, 
	EFI_PXE_BASE_CODE_UDP_PORT dport,
	int len, void *data)
{
	UINTN size;

	size = len;
	if(eficall(pxe->UdpWrite, pxe, (UINTN)MAY_FRAGMENT, dip, &dport, nil, nil, &sport, nil, nil, &size, data))
		return -1;

	return 0;
}

static int
pxeread(void *f, void *data, int len)
{
	Tftp *t = f;
	int seq, n;

	while(!t->eof && t->rp >= t->ep){
		for(;;){
			n = sizeof(t->pkt);
			if(udpread(&t->dip, &t->sip, &t->dport, t->sport, &n, t->pkt))
				continue;
			if(n >= 4)
				break;
		}
		switch(nhgets(t->pkt)){
		case Tftp_DATA:
			seq = nhgets(t->pkt+2);
			if(seq > t->seq){
				putc('?');
				continue;
			}
			hnputs(t->pkt, Tftp_ACK);
			while(udpwrite(&t->dip, t->sport, t->dport, 4, t->pkt))
				putc('!');
			if(seq < t->seq){
				putc('@');
				continue;
			}
			t->seq = seq+1;
			n -= 4;
			t->rp = t->pkt + 4;
			t->ep = t->rp + n;
			t->eof = n < Segsize;
			break;
		case Tftp_ERROR:
			print(t->pkt+4);
			print("\n");
		default:
			t->eof = 1;
			return -1;
		}
		break;
	}
	n = t->ep - t->rp;
	if(len > n)
		len = n;
	memmove(data, t->rp, len);
	t->rp += len;
	return len;
}

static void
pxeclose(void *f)
{
	Tftp *t = f;
	t->eof = 1;
}


static int
tftpopen(Tftp *t, char *path)
{
	static EFI_PXE_BASE_CODE_UDP_PORT xport = 6666;
	int r, n;
	char *p;

	t->sport = xport++;
	t->dport = 0;
	t->rp = t->ep = 0;
	t->seq = 1;
	t->eof = 0;
	t->nul = 0;
	p = t->pkt;
	hnputs(p, Tftp_READ); p += 2;
	n = strlen(path)+1;
	memmove(p, path, n); p += n;
	memmove(p, "octet", 6); p += 6;
	n = p - t->pkt;
	for(;;){
		if(r = udpwrite(&t->dip, t->sport, TftpPort, n, t->pkt))
			break;
		if(r = pxeread(t, 0, 0))
			break;
		return 0;
	}
	pxeclose(t);
	return r;
}

static void*
pxeopen(char *name)
{
	static uchar buf[sizeof(Tftp)+8];
	Tftp *t = (Tftp*)((uintptr)(buf+7)&~7);

	memset(t, 0, sizeof(Tftp));
	memmove(&t->sip, myip, sizeof(myip));
	memmove(&t->dip, serverip, sizeof(serverip));
	if(tftpopen(t, name))
		return nil;
	return t;
}

static int
parseipv6(uchar to[16], char *from)
{
	int i, dig, elipsis;
	char *p;

	elipsis = 0;
	memset(to, 0, 16);
	for(i = 0; i < 16; i += 2){
		dig = 0;
		for(p = from;; p++){
			if(*p >= '0' && *p <= '9')
				dig = (dig << 4) | (*p - '0');
			else if(*p >= 'a' && *p <= 'f')
				dig = (dig << 4) | (*p - 'a'+10);
			else if(*p >= 'A' && *p <= 'F')
				dig = (dig << 4) | (*p - 'A'+10);
			else
				break;
			if(dig > 0xFFFF)
				return -1;
		}
		to[i]   = dig>>8;
		to[i+1] = dig;
		if(*p == ':'){
			if(*++p == ':'){	/* :: is elided zero short(s) */
				if (elipsis)
					return -1;	/* second :: */
				elipsis = i+2;
				p++;
			}
		} else if (p == from)
			break;
		from = p;		
	}
	if(i < 16){
		memmove(&to[elipsis+16-i], &to[elipsis], i-elipsis);
		memset(&to[elipsis], 0, 16-i);
	}
	return 0;
}

static void
parsedhcp(EFI_PXE_BASE_CODE_DHCPV4_PACKET *dhcp)
{
	uchar *p, *e;
	char *x;
	int opt;
	int len;

	memset(mymac, 0, sizeof(mymac));
	memset(serverip, 0, sizeof(serverip));

	/* DHCPv4 */
	if(pxe->Mode->UsingIpv6 == 0){
		memmove(mymac, dhcp->BootpHwAddr, 6);
		memmove(serverip, dhcp->BootpSiAddr, 4);
		return;
	}

	/* DHCPv6 */
	e = (uchar*)dhcp + sizeof(*dhcp);
	p = (uchar*)dhcp + 4;
	while(p+4 <= e){
		opt = p[0]<<8 | p[1];
		len = p[2]<<8 | p[3];
		p += 4;
		if(p + len > e)
			break;
		switch(opt){
		case 1:	/* Client DUID */
			memmove(mymac, p+len-6, 6);
			break;
		case 59: /* Boot File URL */
			for(x = (char*)p; x < (char*)p+len; x++){
				if(*x == '['){
					parseipv6(serverip, x+1);
					break;
				}
			}
			break;
		}
		p += len;
	}
}

int
pxeinit(void **pf)
{
	EFI_PXE_BASE_CODE_DHCPV4_PACKET	*dhcp;
	EFI_PXE_BASE_CODE_MODE *mode;
	EFI_HANDLE *Handles;
	UINTN Count;
	int i;

	pxe = nil;
	Count = 0;
	Handles = nil;
	if(eficall(ST->BootServices->LocateHandleBuffer,
		ByProtocol, &EFI_PXE_BASE_CODE_PROTOCOL_GUID, nil, &Count, &Handles))
		return -1;

	for(i=0; i<Count; i++){
		pxe = nil;
		if(eficall(ST->BootServices->HandleProtocol,
			Handles[i], &EFI_PXE_BASE_CODE_PROTOCOL_GUID, &pxe))
			continue;
		mode = pxe->Mode;
		if(mode == nil || mode->Started == 0)
			continue;
		if(mode->DhcpAckReceived){
			dhcp = (EFI_PXE_BASE_CODE_DHCPV4_PACKET*)mode->DhcpAck;
			goto Found;
		}
		if(mode->PxeReplyReceived){
			dhcp = (EFI_PXE_BASE_CODE_DHCPV4_PACKET*)mode->PxeReply;
			goto Found;
		}
	}
	return -1;

Found:
	parsedhcp(dhcp);
	memmove(myip, mode->StationIp, 16);

	open = pxeopen;
	read = pxeread;
	close = pxeclose;

	if(pf != nil){
		char ini[24];

		memmove(ini, "/cfg/pxe/", 9);
		for(i=0; i<6; i++){
			ini[9+i*2+0] = hex[mymac[i] >> 4];
			ini[9+i*2+1] = hex[mymac[i] & 0xF];
		}
		ini[9+12] = '\0';
		if((*pf = pxeopen(ini)) == nil)
			*pf = pxeopen("/cfg/pxe/default");
	}

	return 0;
}
