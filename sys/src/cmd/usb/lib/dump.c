#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "usb.h"

int verbose;

typedef struct Flags Flags;
typedef struct Classes Classes;

struct Flags {
	int	bit;
	char*	name0;
	char*	name1;
};

struct Classes {
	char*	name;
	struct {
		char*	name;
		char*	proto[4];
	} subclass[4];
};

static Classes	classname[] = {
	[CL_AUDIO]		{"audio", {[1] {"control"}, [2] {"stream"}, [3]{"midi"}}},
	[CL_COMMS]		{"comms", {[1] {"abstract", {[1]"AT"}}}},
	[CL_HID]		{"hid", {[1]{"boot", {[1]"kbd", [2]"mouse"}}}},
	[CL_PRINTER]	{"printer", {[1]"printer", {[1] "uni", [2]"bi"}}},
	[CL_HUB]		{"hub", {[1]{"hub"}}},
	[CL_DATA]		{"data"},
};

static	void	pflag(Flags*, uint);

char *
sclass(ulong csp)
{
	Classes *cs;
	int n;
	static char buf[64];
	int c, s, p;

	c = Class(csp);
	s = Subclass(csp);
	p = Proto(csp);
	if(c < 0 || c >= nelem(classname) || (cs = &classname[c])->name == nil){
		sprint(buf, "%d.%d.%d", c, s, p);
		return buf;
	}
	n = sprint(buf, "%s.", cs->name);
	if(s < 0 || s >= nelem(cs->subclass) || cs->subclass[s].name == nil)
		sprint(buf+n, "%d.%d", s, p);
	else{
		n += sprint(buf+n, "%s.", cs->subclass[s].name);
		if(p < 0 || p >= nelem(cs->subclass[s].proto) || cs->subclass[s].proto[p] == nil)
			sprint(buf+n, "%d", p);
		else
			sprint(buf+n, "%s", cs->subclass[s].proto[p]);
	}
	return buf;
}

void
pdevice(Device *, int, ulong, void *b, int n)
{
	DDevice *d;

	if(n < DDEVLEN)
		return;
	d = b;
	if (debug & Dbginfo) {
		fprint(2, "usb (bcd)%c%c%c%c",
				'0'+((d->bcdUSB[1]>>4)&0xf), '0'+(d->bcdUSB[1]&0xf),
				'0'+((d->bcdUSB[0]>>4)&0xf), '0'+(d->bcdUSB[0]&0xf));
		fprint(2, " class %d subclass %d proto %d [%s] max0 %d",
			d->bDeviceClass, d->bDeviceSubClass, d->bDeviceProtocol,
			sclass(CSP(d->bDeviceClass, d->bDeviceSubClass, d->bDeviceProtocol)),
			d->bMaxPacketSize0);
		fprint(2, " vendor %#x product %#x device (bcd)%c%c%c%c",
			GET2(d->idVendor), GET2(d->idProduct),
			'0'+((d->bcdDevice[1]>>4)&0xf), '0'+(d->bcdDevice[1]&0xf),
			'0'+((d->bcdDevice[0]>>4)&0xf), '0'+(d->bcdDevice[0]&0xf));
		fprint(2, " man %d prod %d serial %d nconfig %d",
			d->iManufacturer, d->iProduct, d->iSerialNumber,
			d->bNumConfigurations);
	}
}

void
phid(Device *, int, ulong, void *b, int n)
{
	DHid *d;

	if(n < DHIDLEN){
		fprint(2, "hid too short\n");
		return;
	}
	d = b;
	if (debug & Dbginfo)
		fprint(2, "HID (bcd)%c%c%c%c country %d nhidclass %d classdtype %#x dlen %d\n",
			'0'+((d->bcdHID[1]>>4)&0xf), '0'+(d->bcdHID[1]&0xf),
			'0'+((d->bcdHID[0]>>4)&0xf), '0'+(d->bcdHID[0]&0xf),
			d->bCountryCode, d->bNumDescriptors,
			d->bClassDescriptorType, GET2(d->wItemLength));
}

static	Flags	ioflags[] = {
	{0, "Data", "Constant"},
	{1, "Array", "Variable"},
	{2, "Absolute", "Relative"},
	{3, "NoWrap", "Wrap"},
	{4, "Linear", "NonLinear"},
	{5, "PreferredState", "No Preferred State"},
	{6, "No Null position", "Null state"},
	{7, "Non Volatile", "Volatile"},
	{8, "Bit Field", "Buffered Bytes"},
	{-1, nil, nil},
};

static void
pflag(Flags *tab, uint v)
{
	char buf[200], *s;
	int n;

	n = 0;
	buf[0] = 0;
	for(; tab->name0 != nil; tab++){
		if(v & (1<<tab->bit))
			s = tab->name1;
		else
			s = tab->name0;
		if(s != nil && *s)
			n += snprint(buf+n, sizeof(buf)-n, ", %s", s);
	}
	if((debug & Dbginfo) && buf[0])
		fprint(2, "[%s]", buf+2);
}

void
preport(Device *, int, ulong, byte *b, int n)
{
	byte *s, *es;
	int tag, nb, i, indent;
	int v;
	Flags *tab;

	s = b+2;
	es = b+n;
	indent = 0;
	while(s < es){
		for(i=0; i<indent; i++)
			fprint(2, " ");
		tag = *s++;
		if(tag == Tlong){
			fprint(2, "long report tag");
			return;
		}
		if((nb = tag&3)==3)
			nb = 4;
		v = 0;
		for(i=0; i<nb; i++)
			v |= s[i]<<(i*8);
		switch(tag & Tmtype){
		case Treserved:
			if(tag == Tlong){
				fprint(2, "long report tag");
				return;
			}
			fprint(2, "illegal tag");
			return;
		case Tmain:
			tab = nil;
			if (debug & Dbginfo) {
				switch(tag & Tmitem){
				case Tinput:	fprint(2, "Input"); tab = ioflags; break;
				case Toutput:	fprint(2, "Output"); tab = ioflags; break;
				case Tfeature:	fprint(2, "Feature"); tab = ioflags; break;
				case Tcoll:	fprint(2, "Collection"); indent++; break;
				case Tecoll:	fprint(2, "End Collection"); indent--; break;
				default:		fprint(2, "unexpected item %.2x", tag);
				}
				fprint(2, "=%#ux", v);
				if(tab != nil)
					pflag(tab, v);
			}
			break;
		case Tglobal:
			if (debug & Dbginfo) {
				fprint(2, "Global %#ux: ", v);
				switch(tag & Tmitem){
				case Tusagepage:
					fprint(2, "Usage Page %#ux", v);
					break;
				case Tlmin:
					fprint(2, "Logical Min %d", v);
					break;
				case Tlmax:
					fprint(2, "Logical Max %d", v);
					break;
				case Tpmin:
					fprint(2, "Physical Min %d", v);
					break;
				case Tpmax:
					fprint(2, "Physical Max %d", v);
					break;
				case Tunitexp:
					fprint(2, "Unit Exponent %d", v);
					break;
				case Tunit:
					fprint(2, "Unit %d", v);
					break;
				case Trepsize:
					fprint(2, "Report size %d", v);
					break;
				case TrepID:
					fprint(2, "Report ID %#x", v);
					break;
				case Trepcount:
					fprint(2, "Report Count %d", v);
					break;
				case Tpush:
					fprint(2, "Push %d", v);
					break;
				case Tpop:
					fprint(2, "Pop %d", v);
					break;
				default:
					fprint(2, "Unknown %#ux", v);
					break;
				}
			}
			break;
		case Tlocal:
			if (debug & Dbginfo) {
				fprint(2, "Local %#ux: ", v);
				switch(tag & Tmitem){
				case Tusage:
					fprint(2, "Usage %d", v);
					break;
				case Tumin:
					fprint(2, "Usage min %d", v);
					break;
				case Tumax:
					fprint(2, "Usage max %d", v);
					break;
				case Tdindex:
					fprint(2, "Designator index %d", v);
					break;
				case Tdmin:
					fprint(2, "Designator min %d", v);
					break;
				case Tdmax:
					fprint(2, "Designator max %d", v);
					break;
				case Tsindex:
					fprint(2, "String index %d", v);
					break;
				case Tsmin:
					fprint(2, "String min %d", v);
					break;
				case Tsmax:
					fprint(2, "String max %d", v);
					break;
				case Tsetdelim:
					fprint(2, "Set delim %#ux", v);
					break;
				default:
					fprint(2, "Unknown %#ux", v);
					break;
				}
			}
			break;
		}
		fprint(2, "\n");
		s += nb;
	}
}

void
phub(Device *, int, ulong, void *b, int n)
{
	DHub *d;

	if(n < DHUBLEN)
		return;
	d = b;
	if (debug & Dbginfo)
		fprint(2, "nport %d charac %#.4x pwr %dms current %dmA remov %#.2x",
			d->bNbrPorts, GET2(d->wHubCharacteristics), d->bPwrOn2PwrGood*2,
			d->bHubContrCurrent, d->DeviceRemovable[0]);
}

void
pstring(Device *, int, ulong, void *b, int n)
{
	byte *rb;
	char *s;
	Rune r;
	int l;

	if(n <= 2){
		fprint(2, "\"\"");
		return;
	}
	if(n & 1){
		fprint(2, "illegal count\n");
		return;
	}
	n /= 2;
	rb = (byte*)b + 2;
	s = malloc(n*UTFmax+1);
	for(l=0; --n >= 0; rb += 2){
		r = GET2(rb);
		l += runetochar(s+l, &r);
	}
	s[l] = 0;
	fprint(2, "\"%s\"", s);
	free(s);
}

void
pcs_raw(char *tag, byte *b, int n)
{
	int i;

	if (debug & Dbginfo) {
		fprint(2, "%s", tag);
		for(i=2; i<n; i++)
			fprint(2, " %.2x", b[i]);
	}
}

static void
pcs_config(Device *, int, ulong, void *b, int n)
{
	pcs_raw("CS_CONFIG", b, n);
}

static void
pcs_string(Device *, ulong, void *b, int n)
{
	pcs_raw("CS_STRING", b, n);
}

static void
pcs_endpoint(Device *, int, ulong, void *bb, int n)
{
	byte *b = bb;

	if (debug & Dbginfo) {
		switch(b[2]) {
		case 0x01:
			fprint(2, "CS_ENDPOINT for TerminalID %d, delay %d, format_tag %#ux\n",
				b[3], b[4], b[5] | (b[6]<<8));
			break;
		case 0x02:
			fprint(2, "CS_INTERFACE FORMAT_TYPE %d, channels %d, subframesize %d, resolution %d, freqtype %d, ",
				b[3], b[4], b[5], b[6], b[7]);
			fprint(2, "freq0 %d, freq1 %d\n",
				b[8] | (b[9]<<8) | (b[10]<<16), b[11] | (b[12]<<8) | (b[13]<<16));
			break;
		default:
			pcs_raw("CS_INTERFACE", bb, n);
		}
	}
}

static void
pcs_interface(Device *, int n, ulong, void *bb, int nb) {

	if ((debug & Dbginfo) && n >= 0) {
		pcs_raw("CS_INTERFACE", bb, nb);
	}
}

void
pdesc(Device *d, int c, ulong csp, byte *b, int n)
{
	void (*f)(Device *, int, ulong, void*, int);
	int ifc = -1;
	int dalt = -1;
	int ep;
	int class, subclass, proto;
	DConfig *dc;
	DInterface *di;
	DEndpoint *de;
	int i;

	class = Class(csp);

	if (c >= nelem(d->config)) {
		fprint(2, "Too many interfaces (%d of %d)\n",
			c, nelem(d->config));
		return;
	}
	if (debug & Dbginfo)
		fprint(2, "pdesc %d.%d [%d]\n", d->id, c, n);
	for(; n > 2 && b[0] && b[0] <= n; b += b[0]){
		if (debug & Dbginfo)
			fprint(2, "desc %d.%d [%d] %#2.2x: ", d->id, c, b[0], b[1]);
		switch (b[1]) {
		case CONFIGURATION:
			if(b[0] < DCONFLEN)
				return;
			dc = (DConfig*)b;
			d->config[c]->nif = dc->bNumInterfaces;
			d->config[c]->cval = dc->bConfigurationValue;
			d->config[c]->attrib = dc->bmAttributes;
			d->config[c]->milliamps = dc->MaxPower*2;
			d->nif += d->config[c]->nif;
			if (debug & Dbginfo)
				fprint(2, "config %d: tdlen %d ninterface %d iconfig %d attr %#.2x power %dmA\n",
					dc->bConfigurationValue, GET2(dc->wTotalLength),
					dc->bNumInterfaces, dc->iConfiguration, dc->bmAttributes,
					dc->MaxPower*2);
			break;
		case INTERFACE:
			if(n < DINTERLEN)
				return;
			di = (DInterface *)b;
			class = di->bInterfaceClass;
			subclass = di->bInterfaceSubClass;
			proto = di->bInterfaceProtocol;
			csp = CSP(class, subclass, proto);
			if (debug & Dbginfo)
				fprint(2, "interface %d: alt %d nept %d class %#x subclass %#x proto %d [%s] iinterface %d\n",
					di->bInterfaceNumber, di->bAlternateSetting,
					di->bNumEndpoints, class, subclass, proto,
					sclass(csp),
					di->iInterface);
			if (c < 0) {
				fprint(2, "Unexpected INTERFACE message\n");
				return;
			}
			ifc = di->bInterfaceNumber;
			dalt = di->bAlternateSetting;
			if (ifc < 0 || ifc >= nelem(d->config[c]->iface))
				sysfatal("Bad interface number %d", ifc);
			if (dalt < 0 || dalt >= nelem(d->config[c]->iface[ifc]->dalt))
				sysfatal("Bad alternate number %d", dalt);
			if (d->config[c] == nil)
				sysfatal("No config");
			if (ifc == 0) {
				if (c == 0)
					d->ep[0]->csp = csp;
				d->config[c]->csp = csp;
			}
			if (d->config[c]->iface[ifc] == nil) {
				d->config[c]->iface[ifc] = mallocz(sizeof(Dinf), 1);
				d->config[c]->iface[ifc]->csp = csp;
			}
			d->config[c]->iface[ifc]->interface = di->bInterfaceNumber;
			break;
		case ENDPOINT:
			if(n < DENDPLEN)
				return;
			de = (DEndpoint *)b;
			if (debug & Dbginfo) {
				fprint(2, "addr %#2.2x attrib %#2.2x maxpkt %d interval %dms",
					de->bEndpointAddress, de->bmAttributes,
					GET2(de->wMaxPacketSize), de->bInterval);
				if(de->bEndpointAddress & 0x80)
					fprint(2, " [IN]");
				else
					fprint(2, " [OUT]");
				switch(de->bmAttributes&0x33){
				case 0:	fprint(2, " [Control]"); break;
				case 1:	fprint(2, " [Iso]");
						switch(de->bmAttributes&0xc){
						case 0x4:	fprint(2, " [Asynchronous]"); break;
						case 0x8:	fprint(2, " [Adaptive]"); break;
						case 0xc:	fprint(2, " [Synchronous]"); break;
						}
						break;
				case 2:	fprint(2, " [Bulk]"); break;
				case 3:	fprint(2, " [Interrupt]"); break;
				}
				if(b[0] == 9)
					fprint(2, "refresh %d synchaddress %d", b[7], b[8]);
				fprint(2, "\n");
			}
			if (c < 0 || ifc < 0 || dalt < 0) {
				fprint(2, "Unexpected ENDPOINT message\n");
				return;
			}
			if (d->config[c]->iface[ifc] == nil)
				sysfatal("d->config[c]->iface[ifc] == nil");
			if (d->config[c]->iface[ifc]->dalt[dalt] == nil)
				d->config[c]->iface[ifc]->dalt[dalt] = mallocz(sizeof(Dalt),1);
			d->config[c]->iface[ifc]->dalt[dalt]->attrib = de->bmAttributes;
			d->config[c]->iface[ifc]->dalt[dalt]->interval = de->bInterval;
			ep = de->bEndpointAddress & 0xf;
			if (d->ep[ep] == nil)
				d->ep[ep] = newendpt(d, ep, class);
			if(d->ep[ep]->maxpkt < GET2(de->wMaxPacketSize))
				d->ep[ep]->maxpkt = GET2(de->wMaxPacketSize);
			d->ep[ep]->addr = de->bEndpointAddress;
			d->ep[ep]->dir = (de->bEndpointAddress & 0x80) ? Ein : Eout;
			d->ep[ep]->type = de->bmAttributes & 0x03;
			d->ep[ep]->isotype = (de->bmAttributes>>2) & 0x03;
			d->ep[ep]->csp = csp;
			d->ep[ep]->conf = d->config[c];
			d->ep[ep]->iface = d->config[c]->iface[ifc];
			for(i = 0; i < nelem(d->config[c]->iface[ifc]->endpt); i++){
				if(d->config[c]->iface[ifc]->endpt[i] == nil){
					d->config[c]->iface[ifc]->endpt[i] = d->ep[ep];
					break;
				}
			}
			if(i == nelem(d->config[c]->iface[ifc]->endpt))
				fprint(2, "Too many endpoints\n");
			if (d->nif <= ep) d->nif = ep+1;
			break;
		default:
			f = nil;
			if(b[1] < nelem(dprinter))
				f = dprinter[b[1]];
			if(f != nil) {
				(*f)(d, c, (dalt<<24) | (ifc<<16) | (csp&0xffff), b, b[0]);
				if (debug & Dbginfo)
					fprint(2, "\n");
			}
			else {
				if (verbose) {
					int i;

					fprint(2, "(unknown type)");
					for(i=1; i<b[0]; i++)
						fprint(2, " %.2x", b[i]);
					fprint(2, "\n");
				}
				else if (debug & Dbginfo)
					fprint(2, "\n");
			}
		}
		n -= b[0];
	}
}
