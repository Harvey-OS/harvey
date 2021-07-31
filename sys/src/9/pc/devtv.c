/*
  * Driver for Bt848 TV tuner.  
  *
  */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include "io.h"

enum {
	Qdir = 0,
	Qdata,
	Qctl,
	Qregs,

	Brooktree_vid = 0x109e,
	Brooktree_848_did = 0x0350,
	Intel_vid = 0x8086,
	Intel_82437_did = 0x122d,

	K = 1024,
	M = K * K,

	Numring = 16,

	ntsc_rawpixels = 910,
	ntsc_sqpixels = 780,					// Including blanking & inactive
	ntsc_hactive = 640,
	ntsc_vactive = 480,
	ntsc_clkx1delay = 135,				// Clock ticks.
	ntsc_clkx1hactive = 754,
	ntsc_vdelay = 26,					// # of scan lines.
	ntsc_vscale = 0,

	i2c_timing = 7 << 4,
	i2c_bt848w3b = 1 << 2,
	i2c_bt848scl = 1 << 1,
	i2c_bt848sda = 1 << 0,

	i2c_miroproee = 0x80,				// MIRO PRO EEPROM
	i2c_tea6300 = i2c_miroproee,
	i2c_tda8425 = 0x82,
	i2c_tda9840 = 0x84,
	i2c_tda9850 = 0xb6,
	i2c_haupee = 0xa0,					// Hauppage EEPROM
	i2c_stbee = 0xae,					// STB EEPROM

	Bt848_miropro = 0,
	Bt848_miro,

	// Bit fields.
	iform_muxsel1 = 3 << 5,				// 004
	iform_muxsel0 = 2 << 5,	
	iform_xtselmask = 3 << 3,
	iform_xtauto = 3 << 3,
	iform_formatmask = 7 << 0,
	iform_ntsc = 1 << 0,

	control_ldec = 1 << 5,				// 02C
	contrast_100percent = 0xd8,			// 030

	vscale_interlaced = 1 << 5,			// 04C

	adelay_ntsc = 104,					// 060
	bdelay_ntsc = 93,					// 064
	adc_crush = 1 << 0,					// 068

	colorfmt_rgb16 = (2 << 4) | (2 << 0),	// 0D4
	colorctl_gamma = 1 << 4,			// 0D8
	capctl_fullframe = 1 << 4,				// 0DC
	capctl_captureodd = 1 << 1,
	capctl_captureeven = 1 << 0,
	vbipacksize = 0x190,				// 0E0

	intstat_riscstatshift = 28,				// 100
	intstat_i2crack = 1 << 25,
	intstat_scerr = 1 << 19,
	intstat_ocerr = 1 << 18,
	intstat_fbus = 1 << 12,
	intstat_risci = 1 << 11,
	intstat_i2cdone = 1 << 8,	
	intstat_vpress = 1 << 5,
	intstat_hlock = 1 << 4,
	intstat_vsync = 1 << 1,
	intstat_fmtchg = 1 << 0,
	intmask_etbf = 1 << 23,				// 104

	gpiodmactl_pltp23_16 = 2 << 6,		// 10C
	gpiodmactl_pltp23_0 = 0 << 6,	
	gpiodmactl_pltp1_16 = 2 << 4,
	gpiodmactl_pltp1_0 = 0 << 4,
	gpiodmactl_pktp_32 = 3 << 2,		
	gpiodmactl_pktp_0 = 0 << 2,		
	gpiodmactl_riscenable = 1 << 1,
	gpiodmactl_fifoenable = 1 << 0,	

	// RISC instructions and parameters.
	fifo_vre = 0x4,
	fifo_vro = 0xC,
	fifo_fm1 = 0x6,

	riscirq = 1 << 24,
	riscwrite = 0x1 << 28,
		riscwrite_sol = 1 << 27,
		riscwrite_eol = 1 << 26,
	riscskip = 0x2 << 28,
	riscjmp = 0x7 << 28,
	riscsync = 0x8 << 28,
		riscsync_resync = 1 << 15,
		riscsync_vre = fifo_vre << 0,
		riscsync_vro = fifo_vro << 0,
		riscsync_fm1 = fifo_fm1 << 0,
	risclabelshift_set = 16,
	risclabelshift_reset = 20,
};

typedef struct {
	ushort		vid;
	ushort		did;
	char			*name;
} Variant;

typedef struct {
	ulong	devstat;		// 000
	ulong	iform;		// 004
	ulong	tdec;			// 008
	ulong	ecrop;		// 00C
	ulong	evdelaylo;		// 010
	ulong	evactivelo;	// 014
	ulong	ehdelaylo;		// 018
	ulong	ehactivelo;	// 01C
	ulong	ehscalehi;		// 020
	ulong	ehscalelo;		// 024
	ulong	bright;		// 028
	ulong	econtrol;		// 02C
	ulong	contrastlo;	// 030
	ulong	satulo;		// 034
	ulong	satvlo;		// 038
	ulong	hue;			// 03C
	ulong	escloop;		// 040
	ulong	pad0;		// 044
	ulong	oform;		// 048
	ulong	evscalehi;		// 04C
	ulong	evscalelo;		// 050
	ulong	test;			// 054
	ulong	pad1[2];		// 058-05C
	ulong	adelay;		// 060
	ulong	bdelay;		// 064
	ulong	adc;			// 068
	ulong	evtc;			// 06C
	ulong	pad2[3];		// 070-078
	ulong	sreset;		// 07C
	ulong	tglb;			// 080
	ulong	tgctrl;		// 084
	ulong	pad3;		// 088
	ulong	ocrop;		// 08C
	ulong	ovdelaylo;		// 090
	ulong	ovactivelo;	// 094
	ulong	ohdelaylo;	// 098
	ulong	ohactivelo;	// 09C
	ulong	ohscalehi;		// 0A0
	ulong	ohscalelo;		// 0A4
	ulong	pad4;		// 0A8
	ulong	ocontrol;		// 0AC
	ulong	pad5[4];		// 0B0-0BC
	ulong	oscloop;		// 0C0
	ulong	pad6[2];		// 0C4-0C8
	ulong	ovscalehi;		// 0CC
	ulong	ovscalelo;		// 0D0
	ulong	colorfmt;		// 0D4
	ulong	colorctl;		// 0D8
	ulong	capctl;		// 0DC
	ulong	vbipacksize;	// 0E0
	ulong	vbipackdel;	// 0E4
	ulong	fcap;			// 0E8
	ulong	ovtc;			// 0EC
	ulong	pllflo;		// 0F0
	ulong	pllfhi;		// 0F4
	ulong	pllxci;		// 0F8
	ulong	dvsif;		// 0FC
	ulong	intstat;		// 100
	ulong	intmask;		// 104
	ulong	pad7;		// 108
	ulong	gpiodmactl;	// 10C
	ulong	i2c;			// 110
	ulong	riscstrtadd;	// 114
	ulong	gpioouten;	// 118
	ulong	gpioreginp;	// 11C
	ulong	risccount;		// 120
	ulong	pad8[55];		// 124-1FC	
	ulong	gpiodata[64];	// 200-2FC
} Bt848;

typedef struct {
	char		*name;
  	ushort	freq_vhfh;	// Start frequency
	ushort	freq_uhf;  
	uchar	VHF_L;
	uchar	VHF_H;
	uchar	UHF;
	uchar	cfg; 
	ushort	offs;
} Tuner;

typedef struct {
	ulong	*fstart;
	ulong	*fjmp;
	uchar	*fbase;
} Frame;

typedef struct {
	Lock;
	Bt848	*bt848;
	Variant	*variant;
	Tuner	*tuner;
	Pcidev	*pci;
	uchar	i2ctuneraddr;
	uchar	i2ccmd;			// I2C command
	int		board;			// What board is this?

	Ref		fref;				// Copying images?
	int		nframes;			// Number of frames to capture.
	Frame	*frames;			// DMA program
	int		lframe;			// Last frame DMAed
} Tv;

enum {
	TemicPAL = 0,
	PhilipsPAL,
	PhilipsNTSC,
	PhilipsSECAM,
	Notuner,
	PhilipsPALI,
	TemicNTSC,
	TemicPALI,
	Temic4036,
	AlpsTSBH1,
	AlpsTSBE1,

	Freqmultiplier = 16,
};

static Tuner tuners[] = {
        {"Temic PAL", Freqmultiplier * 140.25, Freqmultiplier * 463.25, 
		0x02, 0x04, 0x01, 0x8e, 623 },
	{"Philips PAL_I", Freqmultiplier * 140.25, Freqmultiplier * 463.25, 
		0xa0, 0x90, 0x30, 0x8e, 623 },
	{"Philips NTSC",  Freqmultiplier * 157.25, Freqmultiplier * 451.25, 
		0xA0, 0x90, 0x30, 0x8e, 732 },
	{"Philips SECAM", Freqmultiplier * 168.25, Freqmultiplier * 447.25, 
		0xA7, 0x97, 0x37, 0x8e, 623 },
	{"NoTuner", 0, 0, 
		0x00, 0x00, 0x00, 0x00, 0 },
	{"Philips PAL", Freqmultiplier * 168.25, Freqmultiplier * 447.25, 
		0xA0, 0x90, 0x30, 0x8e, 623 },
	{"Temic NTSC", Freqmultiplier * 157.25, Freqmultiplier * 463.25, 
		0x02, 0x04, 0x01, 0x8e, 732 },
	{"TEMIC PAL_I", Freqmultiplier * 170.00, Freqmultiplier * 450.00, 
		0x02, 0x04, 0x01, 0x8e, 623 },
	{"Temic 4036 FY5 NTSC", Freqmultiplier * 157.25, Freqmultiplier * 463.25, 
		0xa0, 0x90, 0x30, 0x8e, 732 },
	{"Alps TSBH1", Freqmultiplier * 137.25, Freqmultiplier * 385.25, 
		0x01, 0x02, 0x08, 0x8e, 732 },
	{"Alps TSBE1", Freqmultiplier * 137.25, Freqmultiplier * 385.25, 
		0x01, 0x02, 0x08, 0x8e, 732 },
};

enum {
	CMvstart,
	CMvgastart,
	CMvstop,
	CMchannel,
};

static Cmdtab tvctlmsg[] = {
	CMvstart,			"vstart",			2,
	CMvgastart,		"vgastart",		3,
	CMvstop,			"vstop",			1,
	CMchannel,		"channel",			3,
};

static Dirtab tvtab[]={
	".",		{ Qdir, 0, QTDIR },	0,	DMDIR|0555,
	"tv",		{ Qdata, 0 },		0,	0600,
	"tvctl",	{ Qctl, 0 },			0,	0600,
	"tvregs",	{ Qregs, 0 },		0,	0400,
};

static Variant variant[] = {
{	Brooktree_vid,	Brooktree_848_did,	"Brooktree 848 TV tuner",	},
};

static char *boards[] = {
	"MIRO PRO",
	"MIRO",
};

static Tv *tv;

static int i2cread(Tv *, uchar, uchar *);
static int i2cwrite(Tv *, uchar, uchar, uchar, int);
static void tvinterrupt(Ureg *, Tv *);
static void vgastart(Tv *, ulong, int);
static void vstart(Tv *, int, int, int, int);
static void vstop(Tv *);
static void frequency(Tv *, int, int);
static int getbpp(Tv *);

static void
tvinit(void)
{
	Pcidev *pci;
	ulong intmask;

	// Test for a triton memory controller.
	intmask = 0;
	if (pcimatch(nil, Intel_vid, Intel_82437_did))
		intmask = intmask_etbf;

	pci = nil;
	while ((pci = pcimatch(pci, 0, 0)) != nil) {
		int i, t;
		Bt848 *bt848;
		ushort hscale, hdelay;
		uchar v;

		for (i = 0; i != nelem(variant); i++)
			if (pci->vid == variant[i].vid && pci->did == variant[i].did)
				break;
		if (i == nelem(variant))
			continue;

		if (tv) {
			print("#V: Currently there is only support for one TV, ignoring %s\n",
				variant[i].name);
			continue;
		}

		tv = (Tv *)malloc(sizeof(Tv));
		assert(tv);

		tv->variant = &variant[i];
		tv->pci = pci;
		tv->bt848 = (Bt848 *)upamalloc(pci->mem[0].bar & ~0x0F, 4 * K, K);
		if (tv->bt848 == nil)
			panic("#V: Cannot allocate memory for Bt848\n");
		bt848 = tv->bt848;

		// i2c stuff.
		if (pci->did >= 878)
			tv->i2ccmd = 0x83;
		else 
			tv->i2ccmd = i2c_timing | i2c_bt848scl | i2c_bt848sda;

		{
			bt848->gpioouten = 0;

			bt848->gpioouten |= 1 << 5;
			bt848->gpiodata[0] |= 1 << 5;
			microdelay(2500);
			bt848->gpiodata[0] |= 1 << 5;
			microdelay(2500);
		}
		
		if (i2cread(tv, i2c_haupee, &v)) 
			panic("#V: Cannot deal with hauppauge boards\n");
		if (i2cread(tv, i2c_stbee, &v))
			panic("#V: Cannot deal with STB cards\n");
		if (i2cread(tv, i2c_miroproee, &v)) {
			tv->board = Bt848_miropro;
			t = ((bt848->gpiodata[0] >> 10) - 1) & 7;
		}
		else {
			tv->board = Bt848_miro;
			t = ((bt848->gpiodata[0] >> 10) - 1) & 7;
		}

		if (t >= nelem(tuners))
			t = 4;
		tv->tuner = &tuners[t];
		tv->i2ctuneraddr = 
			i2cread(tv, 0xc1, &v)? 0xc0:
			i2cread(tv, 0xc3, &v)? 0xc2:
			i2cread(tv, 0xc5, &v)? 0xc4:
			i2cread(tv, 0xc7, &v)? 0xc6: -1;

		bt848->capctl = capctl_fullframe;
		bt848->adelay = adelay_ntsc;
		bt848->bdelay = bdelay_ntsc;
		bt848->iform = iform_muxsel0|iform_xtauto|iform_ntsc;
		bt848->vbipacksize = vbipacksize & 0xff;
		bt848->vbipackdel = (vbipacksize >> 8) & 1;

		// setpll(bt848);

		bt848->colorfmt = colorfmt_rgb16;

		hscale = (ntsc_rawpixels * 4096) / ntsc_sqpixels - 4096;
		hdelay = (ntsc_clkx1delay * ntsc_hactive) / ntsc_clkx1hactive;

		bt848->ovtc = bt848->evtc = 0;
		bt848->ehscalehi = bt848->ohscalehi = (hscale >> 8) & 0xff;
		bt848->ehscalelo = bt848->ohscalelo = hscale & 0xff;
		bt848->evscalehi &= ~0x1f;
		bt848->ovscalehi &= ~0x1f;
		bt848->evscalehi |= vscale_interlaced | ((ntsc_vscale >> 8) & 0x1f);
		bt848->ovscalehi |= vscale_interlaced | (ntsc_vscale >> 8) & 0x1f;
		bt848->evscalelo = bt848->ovscalelo = ntsc_vscale & 0xff;
		bt848->ehactivelo = bt848->ohactivelo = ntsc_hactive & 0xff;
		bt848->ehdelaylo = bt848->ohdelaylo = hdelay & 0xff;
		bt848->evactivelo = bt848->ovactivelo = ntsc_vactive & 0xff;
		bt848->evdelaylo = bt848->ovdelaylo = ntsc_vdelay & 0xff;
		bt848->ecrop = bt848->ocrop = 
			((ntsc_hactive >> 8) & 0x03) |
			((hdelay >> 6) & 0x0C) |
	        		((ntsc_vactive >> 4) & 0x30) |
			((ntsc_vdelay >> 2) & 0xC0);	

		bt848->colorctl = colorctl_gamma;
		bt848->capctl = 0;
		bt848->gpiodmactl = gpiodmactl_pltp23_16 |
			gpiodmactl_pltp1_16 | gpiodmactl_pktp_32;
		bt848->gpioreginp = 0;
		bt848->contrastlo = contrast_100percent;
		bt848->bright = 16;
		bt848->adc = (2 << 6) | adc_crush;
		bt848->econtrol = bt848->ocontrol = control_ldec;
		bt848->escloop = bt848->oscloop = 0;
		bt848->intstat = (ulong)-1;
		bt848->intmask = intmask | 
			intstat_vsync | intstat_scerr | intstat_risci | intstat_ocerr | 
			intstat_vpress | intstat_fmtchg | intstat_hlock;

		bt848->gpioouten &= ~0xF;
		bt848->gpioouten |= 0xF;
		bt848->gpiodata[0] &= ~0xF;
		bt848->gpiodata[0] |= 2;	// Enable audio on the MIRO card.

		print("#V: %s (rev %d) (%s/%s) %.8ulX, intl %d\n", 
				tv->variant->name, pci->rid, boards[tv->board],
				tv->tuner->name, pci->mem[0].bar & ~0x0F, pci->intl);

		intrenable(pci->intl, (void (*)(Ureg *, void *))tvinterrupt, 
				tv, pci->tbdf, "tv");	
	}
}

static Chan*
tvattach(char *spec)
{
	return devattach('V', spec);
}

static Walkqid *
tvwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, tvtab, nelem(tvtab), devgen);
}

static int
tvstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, tvtab, nelem(tvtab), devgen);
}

static Chan*
tvopen(Chan *c, int omode)
{
	switch ((int)c->qid.path) {
	case Qdir:
		return devopen(c, omode, tvtab, nelem(tvtab), devgen);
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
tvclose(Chan *)
{}

static long
tvread(Chan *c, void *a, long n, vlong offset)
{
	static char regs[10 * K];
	static int regslen;
	char *e, *p;

	USED(offset);

	switch((int)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, tvtab, nelem(tvtab), devgen);

	case Qdata: {
		uchar *src;
		int bpf, nb;

		bpf = ntsc_hactive * ntsc_vactive * getbpp(tv);

		if (offset >= bpf) 
			return 0;

		nb = n;
		if (offset + nb > bpf)
			nb = bpf - offset;

		ilock(tv);
		if (tv->frames == nil || tv->lframe >= tv->nframes) {
			iunlock(tv);
			return 0;
		}

		src = tv->frames[tv->lframe].fbase;
		incref(&tv->fref);
		iunlock(tv);

		memmove(a, src + offset, nb);
		decref(&tv->fref);
		return nb;
	}

	case Qctl: {
		char str[128];

		snprint(str, sizeof str, "%dx%dx%d\n", 
				ntsc_hactive, ntsc_vactive, getbpp(tv));
		return readstr(offset, a, n, str);
	}
		
	case Qregs:
		if (offset == 0) {
			Bt848 *bt848 = tv->bt848;
			int i;

			e = regs + sizeof(regs);
			p = regs;
			for (i = 0; i < 0x300 >> 2; i++)
				p = seprint(p, e, "%.3X %.8ulX\n", i << 2, ((ulong *)bt848)[i]);
			regslen = p - regs;
		}

		if (offset >= regslen) 
			return 0;
		if (offset + n > regslen)
			n = regslen - offset;

		return readstr(offset, a, n, &regs[offset]);

	default:
		n=0;
		break;
	}
	return n;
}

static long
tvwrite(Chan *c, void *a, long n, vlong)
{
	Cmdbuf *cb;
	Cmdtab *ct;

	switch((int)c->qid.path) {
	case Qctl:
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		ct = lookupcmd(cb, tvctlmsg, nelem(tvctlmsg));
		switch (ct->index) {
		case CMvstart:
			vstart(tv, (int)strtol(cb->f[1], (char **)nil, 0), 
					ntsc_hactive, ntsc_vactive, ntsc_hactive);
			break;

		case CMvgastart:
			vgastart(tv, strtoul(cb->f[1], (char **)nil, 0),
					(int)strtoul(cb->f[2], (char **)nil, 0));
			break;

		case CMvstop:
			vstop(tv);
			break;

		case CMchannel:
			frequency(tv, (int)strtol(cb->f[1], (char **)nil, 0), 
				(int)strtol(cb->f[2], (char **)nil, 0));
			break;
		}
		poperror();
		break;

	default:
		error(Eio);
	}
	return n;
}

Dev tvdevtab = {
	'V',
	"tv",

	devreset,
	tvinit,
	devshutdown,
	tvattach,
	tvwalk,
	tvstat,
	tvopen,
	devcreate,
	tvclose,
	tvread,
	devbread,
	tvwrite,
	devbwrite,
	devremove,
	devwstat,
};

static void
tvinterrupt(Ureg *, Tv *tv)
{
	Bt848 *bt848 = tv->bt848;

	while (1) {
		ulong status;
		uchar fnum;

		status = bt848->intstat;
		fnum = (status >> intstat_riscstatshift) & 0xf;
		status &= bt848->intmask;

		if (status == 0)
			break;

		bt848->intstat = status;

		if ((status & intstat_fmtchg) == intstat_fmtchg) {
			iprint("int: fmtchg\n");
			status &= ~intstat_fmtchg;
		}

		if ((status & intstat_vpress) == intstat_vpress) {
			iprint("int: vpress\n");
			status &= ~intstat_vpress;
		}
		
		if ((status & intstat_vsync) == intstat_vsync)
			status &= ~intstat_vsync;

		if ((status & intstat_scerr) == intstat_scerr) {
			iprint("int: scerr\n");
			bt848->gpiodmactl &= 
				~(gpiodmactl_riscenable|gpiodmactl_fifoenable);
			bt848->gpiodmactl |= gpiodmactl_fifoenable;
			bt848->gpiodmactl |= gpiodmactl_riscenable;
			status &= ~intstat_scerr;
		}

		if ((status & intstat_risci) == intstat_risci) {
			tv->lframe = fnum;
			status &= ~intstat_risci;
		}
			
		if ((status & intstat_ocerr) == intstat_ocerr) {
			iprint("int: ocerr\n");
			status &= ~intstat_ocerr;
		}

		if ((status & intstat_fbus) == intstat_fbus) {
			iprint("int: fbus\n");
			status &= ~intstat_fbus;
		}

		if (status)
			iprint("int: ignored interrupts %.8ulX\n", status);
	}
}

static int
i2cread(Tv *tv, uchar off, uchar *v)
{	
	Bt848 *bt848 = tv->bt848;
	ulong intstat;
	int i;

	bt848->intstat	= intstat_i2cdone;
	bt848->i2c = (off << 24) | tv->i2ccmd;

	intstat = -1;
	for (i = 0; i != 1000; i++) {
		if ((intstat = bt848->intstat) & intstat_i2cdone)
			break;
		microdelay(1000);
	}

	if (i == 1000) {
		print("i2cread: timeout\n");
		return 0;
	}

	if ((intstat & intstat_i2crack) == 0)
		return 0;

	*v = bt848->i2c >> 8;
	return 1;
}

static int
i2cwrite(Tv *tv, uchar off, uchar d1, uchar d2, int both)
{
	Bt848 *bt848 = tv->bt848;
	ulong intstat, data;
	int i;

	bt848->intstat	= intstat_i2cdone;
	data = (off << 24) | (d1 << 16) | tv->i2ccmd;
	if (both)
		data |= (d2 << 8) | i2c_bt848w3b;
	bt848->i2c = data;

	intstat = 0;
	for (i = 0; i != 1000; i++) {
		if ((intstat = bt848->intstat) & intstat_i2cdone)
			break;
		microdelay(1000);
	}

	if (i == 1000) {
		print("i2cread: timeout\n");
		return 0;
	}

	if ((intstat & intstat_i2crack) == 0)
		return 0;

	return 1;
}

static ulong *
riscframe(ulong paddr, int fnum, int w, int h, int stride, ulong **lastjmp)
{
	ulong *p, *pbase;
	int i;

	pbase = p = (ulong *)malloc((h + 6) * 2 * sizeof(ulong));
	assert(p);

	assert(w <= 0x7FF);

	*p++ = riscsync | riscsync_resync | riscsync_vre;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm1;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite | w | riscwrite_sol | riscwrite_eol;
		*p++ = paddr + i * 2 * stride;
	}

	*p++ = riscsync | riscsync_resync | riscsync_vro;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm1;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite | w | riscwrite_sol | riscwrite_eol;
		*p++ = paddr + (i * 2 + 1) * stride;
	}

	// reset status.  you really need two instructions ;-(.
	*p++ = riscjmp | (0xf << risclabelshift_reset); 	
	*p++ = PADDR(p);
	*p++ = riscjmp | riscirq | (fnum << risclabelshift_set);
	*lastjmp = p;

	return pbase;
}

static void
vactivate(Tv *tv, Frame *frames, int nframes)
{
	Bt848 *bt848 = tv->bt848;

	ilock(tv);
	if (tv->frames) {
		iunlock(tv);
		error(Einuse);
	}
	poperror();

	tv->frames = frames;
	tv->nframes = nframes;

	bt848->riscstrtadd = PADDR(tv->frames[0].fstart);
	bt848->capctl |= capctl_captureodd|capctl_captureeven;
	bt848->gpiodmactl |= gpiodmactl_fifoenable;
	bt848->gpiodmactl |= gpiodmactl_riscenable;
	
	iunlock(tv);
}

static void
vstart(Tv *tv, int nframes, int w, int h, int stride)
{
	Frame *frames;
	int bpp, i, bpf;

	if (nframes >= 0x10)
		error(Ebadarg);

	bpp = getbpp(tv);
	bpf = w * h * bpp;

	// Add one as a spare.
	frames = (Frame *)malloc(nframes * sizeof(Frame));
	assert(frames);
	if (waserror()) {
		for (i = 0; i != nframes; i++)
			if (frames[i].fbase)
				free(frames[i].fbase);
		free(frames);
		nexterror();
	}
	memset(frames, 0, nframes * sizeof(Frame));

	for (i = 0; i != nframes; i++) {
		if ((frames[i].fbase = (uchar *)malloc(bpf)) == nil)
			error(Enomem);
		
		frames[i].fstart = riscframe(PADDR(frames[i].fbase), i, 
								w * bpp, h, stride * bpp, 
								&frames[i].fjmp);
	}

	for (i = 0; i != nframes; i++)
		*frames[i].fjmp = 
			PADDR((i == nframes - 1)? frames[0].fstart: frames[i + 1].fstart);

	vactivate(tv, frames, nframes);
}

static void
vgastart(Tv *tv, ulong paddr, int stride)
{
	Frame *frame;

	frame = (Frame *)malloc(sizeof(Frame));
	assert(frame);
	if (waserror()) {
		free(frame);
		nexterror();
	}

	frame->fbase = nil;
	frame->fstart = 
			riscframe(paddr, 0, ntsc_hactive * getbpp(tv), ntsc_vactive, 
					stride * getbpp(tv), 
					&frame->fjmp);
	*frame->fjmp = PADDR(frame->fstart);

	vactivate(tv, frame, 1);
}

static void
vstop(Tv *tv)
{
	Bt848 *bt848 = tv->bt848;

	ilock(tv);
	if (tv->fref.ref > 0) {
		iunlock(tv);
		error(Einuse);
	}

	if (tv->frames) {
		int i;

		bt848->gpiodmactl &= ~gpiodmactl_riscenable;
		bt848->gpiodmactl &= ~gpiodmactl_fifoenable;
		bt848->capctl &= ~(capctl_captureodd|capctl_captureeven);

		for (i = 0; i != tv->nframes; i++)
			if (tv->frames[i].fbase)
				free(tv->frames[i].fbase);
		free(tv->frames);
		tv->frames = nil;
	}
	iunlock(tv);
}

static long
hrcfreq[] = {	/* HRC CATV frequencies */
	    0,  7200,  5400,  6000,  6600,  7800,  8400, 17400,
	18000, 18600, 19200, 19800, 20400, 21000, 12000, 12600,
	13200, 13800, 14400, 15000, 15600, 16200, 16800, 21600,
	22200, 22800, 23400, 24000, 24600, 25200, 25800, 26400,
	27000, 27600, 28200, 28800, 29400, 30000, 30600, 31200,
	31800, 32400, 33000, 33600, 34200, 34800, 35400, 36000,
	36600, 37200, 37800, 38400, 39000, 39600, 40200, 40800,
	41400, 42000, 42600, 43200, 43800, 44400, 45000, 45600,
	46200, 46800, 47400, 48000, 48600, 49200, 49800, 50400,
	51000, 51600, 52200, 52800, 53400, 54000, 54600, 55200,
	55800, 56400, 57000, 57600, 58200, 58800, 59400, 60000,
	60600, 61200, 61800, 62400, 63000, 63600, 64200,  9000,
	 9600, 10200, 10800, 11400, 64800, 65400, 66000, 66600,
	67200, 67800, 68400, 69000, 69600, 70200, 70800, 71400,
	72000, 72600, 73200, 73800, 74400, 75000, 75600, 76200,
	76800, 77400, 78000, 78600, 79200, 79800,
};

static void
frequency(Tv *tv, int channel, int finetune)
{
	Tuner *tuner = tv->tuner;
	long freq;
	ushort div;
	uchar cfg;

	if (channel < 0 || channel > nelem(hrcfreq))
		error(Ebadarg);

	freq = (hrcfreq[channel] * Freqmultiplier) / 100;

	if (freq < tuner->freq_vhfh)
		cfg = tuner->VHF_L;
	else if (freq < tuner->freq_uhf)
		cfg =  tuner->VHF_H;
	else
		cfg = tuner->UHF;

	div = (freq + tuner->offs + finetune) & 0x7fff;
	
	if (!i2cwrite(tv, 0xc0, (div >> 8) & 0x7f, div, 1))
		error(Eio);
	
	if (!i2cwrite(tv, 0xc0, tuner->cfg, cfg, 1))
		error(Eio);
}

static int
getbpp(Tv *tv)
{
	switch (tv->bt848->colorfmt) {
	case colorfmt_rgb16:
		return 2;
	default:
		error("getbpp: Unsupport color format\n");
	}	
	return -1;
}

	