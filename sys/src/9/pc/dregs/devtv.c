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
#include "hcwAMC.h"

#define max(a, b)	(((a) > (b))? (a): (b))

enum {
	Qdir = 0,
	Qsubdir,
	Qsubbase,
	Qvdata = Qsubbase,
	Qadata,
	Qctl,
	Qregs,

	Brooktree_vid = 0x109e,
	Brooktree_848_did = 0x0350,
	Brooktree_878_did = 0x036E,
	Intel_vid = 0x8086,
	Intel_82437_did = 0x122d,

	K = 1024,
	M = K * K,

	Ntvs = 4,

	Numring = 16,

	ntsc_rawpixels = 910,
	ntsc_sqpixels = 780,		/* Including blanking & inactive */
	ntsc_hactive = 640,
	ntsc_vactive = 480,
	ntsc_clkx1delay = 135,		/* Clock ticks. */
	ntsc_clkx1hactive = 754,
	ntsc_vdelay = 26,		/* # of scan lines. */
	ntsc_vscale = 0,

	i2c_nostop = 1 << 5,
	i2c_nos1b  = 1 << 4,
	i2c_timing = 7 << 4,
	i2c_bt848w3b = 1 << 2,
	i2c_bt848scl = 1 << 1,
	i2c_bt848sda = 1 << 0,
	i2c_scl = i2c_bt848scl,
	i2c_sda = i2c_bt848sda,

	i2c_miroproee = 0x80,		/* MIRO PRO EEPROM */
	i2c_tea6300 = 0x80,
	i2c_tda8425 = 0x82,
	i2c_tda9840 = 0x84,
	i2c_tda9850 = 0xb6,
	i2c_haupee = 0xa0,		/* Hauppage EEPROM */
	i2c_stbee = 0xae,		/* STB EEPROM */
	i2c_msp3400 = 0x80,

	i2c_timeout = 1000,
	i2c_delay = 10,

	Bt848_miropro = 0,
	Bt848_miro,
	Bt878_hauppauge,

	/* Bit fields. */
	iform_muxsel1 = 3 << 5,		/* 004 */
	iform_muxsel0 = 2 << 5,
	iform_xtselmask = 3 << 3,
	iform_xtauto = 3 << 3,
	iform_formatmask = 7 << 0,
	iform_ntsc = 1 << 0,

	control_ldec = 1 << 5,		/* 02C */
	contrast_100percent = 0xd8,	/* 030 */

	vscale_interlaced = 1 << 5,	/* 04C */

	adelay_ntsc = 104,		/* 060 */
	bdelay_ntsc = 93,		/* 064 */
	adc_crush = 1 << 0,		/* 068 */

	colorfmt_rgb16 = 2 << 4 | 2 << 0,	/* 0D4 */
	colorfmt_YCbCr422 = 8 << 4 | 8 << 0,
	colorfmt_YCbCr411 = 9 << 4 | 9 << 0,
	colorctl_gamma     = 1 << 4,	/* 0D8 */
	capctl_fullframe   = 1 << 4,	/* 0DC */
	capctl_captureodd  = 1 << 1,
	capctl_captureeven = 1 << 0,
	vbipacksize = 0x190,		/* 0E0 */

	intstat_riscstatshift = 28,	/* 100 */
	intstat_i2crack = 1 << 25,
	intstat_scerr = 1 << 19,
	intstat_ocerr = 1 << 18,
	intstat_pabort = 1 << 17,
	intstat_riperr = 1 << 16,
	intstat_pperr = 1 << 15,
	intstat_fdsr = 1 << 14,
	intstat_ftrgt = 1 << 13,
	intstat_fbus = 1 << 12,
	intstat_risci = 1 << 11,
	intstat_i2cdone = 1 << 8,
	intstat_vpress = 1 << 5,
	intstat_hlock = 1 << 4,
	intstat_vsync = 1 << 1,
	intstat_fmtchg = 1 << 0,
	intmask_etbf = 1 << 23,		/* 104 */

	gpiodmactl_apwrdn = 1 << 26,	/* 10C */
	gpiodmactl_daes2 = 1 << 13,
	gpiodmactl_daiomda = 1 << 6,
	gpiodmactl_pltp23_16 = 2 << 6,
	gpiodmactl_pltp23_0 = 0 << 6,
	gpiodmactl_pltp1_16 = 2 << 4,
	gpiodmactl_pltp1_0 = 0 << 4,
	gpiodmactl_acapenable = 1 << 4,
	gpiodmactl_pktp_32 = 3 << 2,
	gpiodmactl_pktp_0 = 0 << 2,
	gpiodmactl_riscenable = 1 << 1,
	gpiodmactl_fifoenable = 1 << 0,

	/* RISC instructions and parameters. */
	fifo_vre = 0x4,
	fifo_vro = 0xc,
	fifo_fm1 = 0x6,
	fifo_fm3 = 0xe,

	riscirq = 1 << 24,
	riscwrite = 1 << 28,
	riscwrite123 = 9 << 28,
	riscwrite1s23 = 11 << 28,
		riscwrite_sol = 1 << 27,
		riscwrite_eol = 1 << 26,
	riscskip = 0x2 << 28,
	riscjmp = 0x7 << 28,
	riscsync = 0x8 << 28,
		riscsync_resync = 1 << 15,
		riscsync_vre = fifo_vre << 0,
		riscsync_vro = fifo_vro << 0,
		riscsync_fm1 = fifo_fm1 << 0,
		riscsync_fm3 = fifo_fm3 << 0,
	risclabelshift_set = 16,
	risclabelshift_reset = 20,

	AudioTuner = 0,
	AudioRadio,
	AudioExtern,
	AudioIntern,
	AudioOff,
	AudioOn,

	asel_tv = 0,
	asel_radio,
	asel_mic,
	asel_smxc,

	Hwbase_ad = 448000,

	msp_dem = 0x10,
	msp_bbp = 0x12,

	/* Altera definitions. */
	gpio_altera_data = 1 << 0,
	gpio_altera_clock = 1 << 20,
	gpio_altera_nconfig = 1 << 23,

	Ial = 0x140001,
	Idma = 0x100002,

	Adsp = 0x7fd8,
	Adsp_verifysystem = 1,
	Adsp_querysupportplay,
	Adsp_setstyle,
	Adsp_setsrate,
	Adsp_setchannels,
	Adsp_setresolution,
	Adsp_setcrcoptions,
	Adsp_bufenqfor,
	Adsp_logbuffer,
	Adsp_startplay,
	Adsp_stopplay,
	Adsp_autostop,
	Adsp_startrecord,
	Adsp_stoprecord,
	Adsp_getlastprocessed,
	Adsp_pause,
	Adsp_resume,
	Adsp_setvolume,
	Adsp_querysupportrecord,
	Adsp_generalbufenq,
	Adsp_setdownmixtype,
	Adsp_setigain,
	Adsp_setlineout,
	Adsp_setlangmixtype,

	Kfir_gc = 0,
	Kfir_dsp_riscmc,
	Kfir_dsp_risccram,
	Kfir_dsp_unitmc,
	Kfir_bsm_mc,
	Kfir_mux_mc,

	Kfir_devid_gc = 7,
	Kfir_devid_dsp = 4,
	Kfir_devid_bsm = 5,
	Kfir_devid_mux = 8,

	Kfir_200 = 200,
	Kfir_dev_inst = Kfir_200,
	Kfir_201 = 201,
	Kfir_exec = Kfir_201,
	Kfir_202 = 202,
	Kfir_adr_eready = 254,

	Kfir_d_eready_encoding = 0,
	Kfir_d_eready_ready,
	Kfir_d_eready_test,
	Kfir_d_eready_stopdetect,
	Kfir_d_eready_seqend,

	VT_KFIR_OFF = 0,
	VT_KFIR_ON,

	VT_KFIR_LAYER_II = 1,
	VT_KFIR_STEREO = 1,

	Gpioinit = 0,
	Gpiooutput,
	Gpioinput,

	Srate_5512 = 0,
	Srate_11025 = 2,
	Srate_16000 = 3,
	Srate_22050 = 4,
	Srate_32000 = 5,
	Srate_44100 = 6,
	Srate_48000 = 7,

};

typedef struct Variant Variant;
struct Variant {
	ushort	vid;
	ushort	did;
	char	*name;
};

typedef struct Bt848 Bt848;
struct Bt848 {
	ulong	devstat;	/* 000 */
	ulong	iform;		/* 004 */
	ulong	tdec;		/* 008 */
	ulong	ecrop;		/* 00C */
	ulong	evdelaylo;	/* 010 */
	ulong	evactivelo;	/* 014 */
	ulong	ehdelaylo;	/* 018 */
	ulong	ehactivelo;	/* 01C */
	ulong	ehscalehi;	/* 020 */
	ulong	ehscalelo;	/* 024 */
	ulong	bright;		/* 028 */
	ulong	econtrol;	/* 02C */
	ulong	contrastlo;	/* 030 */
	ulong	satulo;		/* 034 */
	ulong	satvlo;		/* 038 */
	ulong	hue;		/* 03C */
	ulong	escloop;	/* 040 */
	ulong	pad0;		/* 044 */
	ulong	oform;		/* 048 */
	ulong	evscalehi;	/* 04C */
	ulong	evscalelo;	/* 050 */
	ulong	test;		/* 054 */
	ulong	pad1[2];	/* 058-05C */
	ulong	adelay;		/* 060 */
	ulong	bdelay;		/* 064 */
	ulong	adc;		/* 068 */
	ulong	evtc;		/* 06C */
	ulong	pad2[3];	/* 070-078 */
	ulong	sreset;		/* 07C */
	ulong	tglb;		/* 080 */
	ulong	tgctrl;		/* 084 */
	ulong	pad3;		/* 088 */
	ulong	ocrop;		/* 08C */
	ulong	ovdelaylo;	/* 090 */
	ulong	ovactivelo;	/* 094 */
	ulong	ohdelaylo;	/* 098 */
	ulong	ohactivelo;	/* 09C */
	ulong	ohscalehi;	/* 0A0 */
	ulong	ohscalelo;	/* 0A4 */
	ulong	pad4;		/* 0A8 */
	ulong	ocontrol;	/* 0AC */
	ulong	pad5[4];	/* 0B0-0BC */
	ulong	oscloop;	/* 0C0 */
	ulong	pad6[2];	/* 0C4-0C8 */
	ulong	ovscalehi;	/* 0CC */
	ulong	ovscalelo;	/* 0D0 */
	ulong	colorfmt;	/* 0D4 */
	ulong	colorctl;	/* 0D8 */
	ulong	capctl;		/* 0DC */
	ulong	vbipacksize;	/* 0E0 */
	ulong	vbipackdel;	/* 0E4 */
	ulong	fcap;		/* 0E8 */
	ulong	ovtc;		/* 0EC */
	ulong	pllflo;		/* 0F0 */
	ulong	pllfhi;		/* 0F4 */
	ulong	pllxci;		/* 0F8 */
	ulong	dvsif;		/* 0FC */
	ulong	intstat;	/* 100 */
	ulong	intmask;	/* 104 */
	ulong	pad7;		/* 108 */
	ulong	gpiodmactl;	/* 10C */
	ulong	i2c;		/* 110 */
	ulong	riscstrtadd;	/* 114 */
	ulong	gpioouten;	/* 118 */
	ulong	gpioreginp;	/* 11C */
	ulong	risccount;	/* 120 */
	ulong	pad8[55];	/* 124-1FC */
	ulong	gpiodata[64];	/* 200-2FC */
};

#define packetlen	i2c

typedef struct Tuner Tuner;
struct Tuner {
	char	*name;
  	ushort	freq_vhfh;	/* Start frequency */
	ushort	freq_uhf;
	uchar	VHF_L;
	uchar	VHF_H;
	uchar	UHF;
	uchar	cfg;
	ushort	offs;
};

typedef struct Frame Frame;
struct Frame {
	ulong	*fstart;
	ulong	*fjmp;
	uchar	*fbase;
};

typedef struct Tv Tv;
struct Tv {
	Lock;
	Rendez;
	Bt848	*bt848;
	Bt848	*bt878;		/* Really only audio control registers */
	Variant	*variant;
	Tuner	*tuner;
	Pcidev	*pci;
	uchar	i2ctuneraddr;
	uchar	i2ccmd;		/* I2C command */
	int	board;		/* What board is this? */
	ulong	cfmt;		/* Current color format. */
	int	channel;	/* Current channel */
	Ref	fref;		/* Copying images? */
	int	nframes;	/* Number of frames to capture. */
	Frame	*frames;	/* DMA program */
	int	lvframe;	/* Last video frame DMAed */
	uchar	*amux;		/* Audio multiplexer. */
	int	nablocks;	/* Number of audio blocks allocated */
	int	absize;		/* Audio block size */
	int	narblocks;	/* Number of audio blocks received */
	ulong	*arisc;		/* Audio risc bloc */
	uchar	*abuf;		/* Audio data buffers */
	char	ainfo[128];

	/* WinTV/PVR stuff. */
	int	msp;
	Lock	kfirlock;
	ulong	i2cstate;	/* Last i2c state. */
	int	gpiostate;	/* Current GPIO state */
	ulong	alterareg;	/* Last used altera register */
	ulong	alteraclock;	/* Used to clock the altera */
	int	asrate;		/* Audio sample rate */
	uchar	aleft, aright;	/* Left and right audio volume */
	ulong	kfirclock;
	Ref	aref;		/* Copying audio? */
};

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

static int hp_tuners[] = {
	Notuner,
	Notuner,
	Notuner,
	Notuner,
	Notuner,
	PhilipsNTSC,
	Notuner,
	Notuner,
	PhilipsPAL,
	PhilipsSECAM,
	PhilipsNTSC,
	PhilipsPALI,
	Notuner,
	Notuner,
	TemicPAL,
	TemicPALI,
	Notuner,
	PhilipsSECAM,
	PhilipsNTSC,
	PhilipsPALI,
	Notuner,
	PhilipsPAL,
	Notuner,
	PhilipsNTSC,
};

enum {
	CMvstart,
	CMastart,
	CMastop,
	CMvgastart,
	CMvstop,
	CMchannel,
	CMcolormode,
	CMvolume,
	CMmute,
};

static Cmdtab tvctlmsg[] = {
	CMvstart,	"vstart",	2,
	CMastart,	"astart",	5,
	CMastop,	"astop",	1,
	CMvgastart,	"vgastart",	3,
	CMvstop,	"vstop",	1,
	CMchannel,	"channel",	3,
	CMcolormode,	"colormode",	2,
	CMvolume,	"volume",	3,
	CMmute,		"mute",		1,
};

static Variant variant[] = {
	{ Brooktree_vid, Brooktree_848_did, "Brooktree 848 TV tuner", },
	{ Brooktree_vid, Brooktree_878_did, "Brooktree 878 TV tuner", },
};

static char *boards[] = {
	"MIRO PRO",
	"MIRO",
	"Hauppauge Bt878",
};

static ushort Adspfsample[] = {
	0x500, 0x700, 0x400, 0x600, 0x300, 0x200, 0x000, 0x100
};
static ushort Adspstereorates[] = {
	64, 96, 112, 128, 160, 192, 224, 256, 320, 384
};

static uchar miroamux[] = { 2, 0, 0, 0, 10, 0 };
static uchar hauppaugeamux[] = { 0, 1, 2, 3, 4, 0 };
static char *nicamstate[] = {
	"analog", "???", "digital", "bad digital receiption"
};


static Tv tvs[Ntvs];
static int ntvs;

static int i2cread(Tv *, uchar, uchar *);
static int i2cwrite(Tv *, uchar, uchar, uchar, int);
static void tvinterrupt(Ureg *, Tv *);
static void vgastart(Tv *, ulong, int);
static void vstart(Tv *, int, int, int, int);
static void astart(Tv *, char *, uint, uint, uint);
static void vstop(Tv *);
static void astop(Tv *);
static void colormode(Tv *, char *);
static void frequency(Tv *, int, int);
static int getbitspp(Tv *);
static char *getcolormode(ulong);
static int mspreset(Tv *);
static void i2cscan(Tv *);
static int kfirinitialize(Tv *);
static void msptune(Tv *);
static void mspvolume(Tv *, int, int, int);
static void gpioenable(Tv *, ulong, ulong);
static void gpiowrite(Tv *, ulong, ulong);

static void
tvinit(void)
{
	Pcidev *pci;
	ulong intmask;

	/* Test for a triton memory controller. */
	intmask = 0;
	if (pcimatch(nil, Intel_vid, Intel_82437_did))
		intmask = intmask_etbf;

	pci = nil;
	while ((pci = pcimatch(pci, 0, 0)) != nil) {
		int i, t;
		Tv *tv;
		Bt848 *bt848;
		ushort hscale, hdelay;
		uchar v;

		for (i = 0; i != nelem(variant); i++)
			if (pci->vid == variant[i].vid && pci->did == variant[i].did)
				break;
		if (i == nelem(variant))
			continue;

		if (ntvs >= Ntvs) {
			print("#V: Too many TV cards found\n");
			continue;
		}

		tv = &tvs[ntvs++];
		tv->variant = &variant[i];
		tv->pci = pci;
		tv->bt848 = (Bt848 *)vmap(pci->mem[0].bar & ~0x0F, 4 * K);
		if (tv->bt848 == nil)
			panic("#V: Cannot allocate memory for Bt848");
		bt848 = tv->bt848;

		/* i2c stuff. */
		if (pci->did >= 878)
			tv->i2ccmd = 0x83;
		else
			tv->i2ccmd = i2c_timing | i2c_bt848scl | i2c_bt848sda;

		t = 0;
		if (i2cread(tv, i2c_haupee, &v)) {
			uchar ee[256];
			Pcidev *pci878;
			Bt848 *bt878;

			tv->board = Bt878_hauppauge;
			if (!i2cwrite(tv, i2c_haupee, 0, 0, 0))
				panic("#V: Cannot write to Hauppauge EEPROM");
			for (i = 0; i != sizeof ee; i++)
				if (!i2cread(tv, i2c_haupee + 1, &ee[i]))
					panic("#V: Cannot read from Hauppauge EEPROM");

			if (ee[9] > sizeof hp_tuners / sizeof hp_tuners[0])
				panic("#V: Tuner out of range (max %d, this %d)",
					sizeof hp_tuners / sizeof hp_tuners[0], ee[9]);
			t = hp_tuners[ee[9]];

			/* Initialize the audio channel. */
			if ((pci878 = pcimatch(nil, Brooktree_vid, 0x878)) == nil)
				panic("#V: Unsupported Hauppage board");

			tv->bt878 = bt878 =
				(Bt848 *)vmap(pci878->mem[0].bar & ~0x0F, 4 * K);
			if (bt878 == nil)
				panic("#V: Cannot allocate memory for the Bt878");

			kfirinitialize(tv);
			// i2cscan(tv);
			mspreset(tv);

			bt878->gpiodmactl = 0;
			bt878->intstat = (ulong)-1;
			intrenable(pci878->intl, (void (*)(Ureg *, void *))tvinterrupt,
					tv, pci878->tbdf, "tv");

			tv->amux = hauppaugeamux;
		}
		else if (i2cread(tv, i2c_stbee, &v)) {
			USED(t);
			panic("#V: Cannot deal with STB cards\n");
		}
		else if (i2cread(tv, i2c_miroproee, &v)) {
			tv->board = Bt848_miropro;
			t = ((bt848->gpiodata[0] >> 10) - 1) & 7;
			tv->amux = miroamux;
		}
		else {
			tv->board = Bt848_miro;
			tv->amux = miroamux;
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

		tv->cfmt = bt848->colorfmt = colorfmt_rgb16;

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
			intstat_vpress | intstat_fmtchg;


		if (tv->amux) {
			gpioenable(tv, ~0xfff, 0xfff);
			gpiowrite(tv, ~0xfff, tv->amux[AudioRadio]);
		}

		print("#V%ld: %s (rev %d) (%s/%s) intl %d\n",
			tv - tvs, tv->variant->name, pci->rid, boards[tv->board],
			tv->tuner->name, pci->intl);

		intrenable(pci->intl, (void (*)(Ureg *, void *))tvinterrupt,
			tv, pci->tbdf, "tv");
	}
}

static Chan*
tvattach(char *spec)
{
	return devattach('V', spec);
}

#define TYPE(q)		((int)((q).path & 0xff))
#define DEV(q)		((int)(((q).path >> 8) & 0xff))
#define QID(d, t)	((((d) & 0xff) << 8) | (t))

static int
tv1gen(Chan *c, int i, Dir *dp)
{
	Qid qid;

	switch (i) {
	case Qvdata:
		mkqid(&qid, QID(DEV(c->qid), Qvdata), 0, QTFILE);
		devdir(c, qid, "video", 0, eve, 0444, dp);
		return 1;
	case Qadata:
		mkqid(&qid, QID(DEV(c->qid), Qadata), 0, QTFILE);
		devdir(c, qid, "audio", 0, eve, 0444, dp);
		return 1;
	case Qctl:
		mkqid(&qid, QID(DEV(c->qid), Qctl), 0, QTFILE);
		devdir(c, qid, "ctl", 0, eve, 0444, dp);
		return 1;
	case Qregs:
		mkqid(&qid, QID(DEV(c->qid), Qregs), 0, QTFILE);
		devdir(c, qid, "regs", 0, eve, 0444, dp);
		return 1;
	}
	return -1;
}

static int
tvgen(Chan *c, char *, Dirtab *, int, int i, Dir *dp)
{
	Qid qid;
	int dev;

	dev = DEV(c->qid);
	switch (TYPE(c->qid)) {
	case Qdir:
		if (i == DEVDOTDOT) {
			mkqid(&qid, Qdir, 0, QTDIR);
			devdir(c, qid, "#V", 0, eve, 0555, dp);
			return 1;
		}

		if (i >= ntvs)
			return -1;

		mkqid(&qid, QID(i, Qsubdir), 0, QTDIR);
		snprint(up->genbuf, sizeof(up->genbuf), "tv%d", i);
		devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
		return 1;

	case Qsubdir:
		if (i == DEVDOTDOT) {
			mkqid(&qid, QID(dev, Qdir), 0, QTDIR);
			snprint(up->genbuf, sizeof(up->genbuf), "tv%d", dev);
			devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}

		return tv1gen(c, i + Qsubbase, dp);

	case Qvdata:
	case Qadata:
	case Qctl:
	case Qregs:
		return tv1gen(c, TYPE(c->qid), dp);

	default:
		return -1;
	}
}

static Walkqid *
tvwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, tvgen);
}

static int
tvstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, tvgen);
}

static Chan*
tvopen(Chan *c, int omode)
{
	if (omode != OREAD &&
		TYPE(c->qid) != Qctl && TYPE(c->qid) != Qvdata)
		error(Eperm);

	switch (TYPE(c->qid)) {
	case Qdir:
		return devopen(c, omode, nil, 0, tvgen);
	case Qadata:
		if (tvs[DEV(c->qid)].bt878 == nil)
			error(Enonexist);
		break;
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;

	if (TYPE(c->qid) == Qadata)
		c->aux = nil;
	return c;
}

static void
tvclose(Chan *)
{
}

static int
audioblock(void *)
{
	return 1;
}

static long
tvread(Chan *c, void *a, long n, vlong offset)
{
	static char regs[10 * K];
	static int regslen;
	Tv *tv;
	char *e, *p;
	uchar *src;

	USED(offset);

	switch(TYPE(c->qid)) {
	case Qdir:
	case Qsubdir:
		return devdirread(c, a, n, 0, 0, tvgen);

	case Qvdata: {
		int bpf, nb;

		tv = &tvs[DEV(c->qid)];
		bpf = ntsc_hactive * ntsc_vactive * getbitspp(tv) / 8;

		if (offset >= bpf)
			return 0;

		nb = n;
		if (offset + nb > bpf)
			nb = bpf - offset;

		ilock(tv);
		if (tv->frames == nil || tv->lvframe >= tv->nframes ||
			tv->frames[tv->lvframe].fbase == nil) {
			iunlock(tv);
			return 0;
		}

		src = tv->frames[tv->lvframe].fbase;
		incref(&tv->fref);
		iunlock(tv);

		memmove(a, src + offset, nb);
		decref(&tv->fref);
		return nb;
	}

	case Qadata: {
		ulong uablock = (ulong)c->aux, bnum, tvablock;
		int boffs, nbytes;

		tv = &tvs[DEV(c->qid)];
		if (tv->bt878 == nil)
			error("#V: No audio device");
		if (tv->absize == 0)
			error("#V: audio not initialized");

		bnum = offset / tv->absize;
		boffs = offset % tv->absize;
		nbytes = tv->absize - boffs;

		incref(&tv->aref);
		for (;;) {
			tvablock = tv->narblocks;	/* Current tv block. */

			if (uablock == 0)
				uablock = tvablock - 1;

			if (tvablock >= uablock + bnum + tv->narblocks)
				uablock = tvablock - 1 - bnum;

			if (uablock + bnum == tvablock) {
				sleep(tv, audioblock, nil);
				continue;
			}
			break;
		}

		print("uablock %ld, bnum %ld, boffs %d, nbytes %d, tvablock %ld\n",
			uablock, bnum, boffs, nbytes, tvablock);
		src = tv->abuf + ((uablock + bnum) % tv->nablocks) * tv->absize;
		print("copying from %#p (abuf %#p), nbytes %d (block %ld.%ld)\n",
			src + boffs, tv->abuf, nbytes, uablock, bnum);

		memmove(a, src + boffs, nbytes);
		decref(&tv->aref);

		uablock += (boffs + nbytes) % tv->absize;
		c->aux = (void*)uablock;

		return nbytes;
	}

	case Qctl: {
		char str[128];

		tv = &tvs[DEV(c->qid)];
		snprint(str, sizeof str, "%dx%dx%d %s channel %d %s\n",
			ntsc_hactive, ntsc_vactive, getbitspp(tv),
			getcolormode(tv->cfmt), tv->channel, tv->ainfo);
		return readstr(offset, a, strlen(str) + 1, str);
	}

	case Qregs:
		if (offset == 0) {
			Bt848 *bt848;
			int i;

			tv = &tvs[DEV(c->qid)];
			bt848 = tv->bt848;

			e = regs + sizeof(regs);
			p = regs;
			for (i = 0; i < 0x300 >> 2; i++)
				p = seprint(p, e, "%.3X %.8ulX\n", i << 2,
					((ulong *)bt848)[i]);
			if (tv->bt878) {
				bt848 = tv->bt878;

				for (i = 0; i < 0x300 >> 2; i++)
					p = seprint(p, e, "%.3X %.8ulX\n",
						i << 2, ((ulong *)bt848)[i]);
			}

			regslen = p - regs;
		}

		if (offset >= regslen)
			return 0;
		if (offset + n > regslen)
			n = regslen - offset;

		return readstr(offset, a, n, &regs[offset]);

	default:
		n = 0;
		break;
	}
	return n;
}

static long
tvwrite(Chan *c, void *a, long n, vlong)
{
	Cmdbuf *cb;
	Cmdtab *ct;
	Tv *tv;

	tv = &tvs[DEV(c->qid)];
	switch(TYPE(c->qid)) {
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

		case CMastart:
			astart(tv, cb->f[1], (uint)strtol(cb->f[2], (char **)nil, 0),
				(uint)strtol(cb->f[3], (char **)nil, 0),
				(uint)strtol(cb->f[4], (char **)nil, 0));
			break;

		case CMastop:
			astop(tv);
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

		case CMcolormode:
			colormode(tv, cb->f[1]);
			break;

		case CMvolume:
			if (!tv->msp)
				error("#V: No volume control");

			mspvolume(tv, 0, (int)strtol(cb->f[1], (char **)nil, 0),
				(int)strtol(cb->f[2], (char **)nil, 0));
			break;

		case CMmute:
			if (!tv->msp)
				error("#V: No volume control");

			mspvolume(tv, 1, 0, 0);
			break;
		}
		poperror();
		free(cb);
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
	Bt848 *bt848 = tv->bt848, *bt878 = tv->bt878;

	for (;;) {
		ulong vstat, astat;
		uchar fnum;

		vstat = bt848->intstat;
		fnum = (vstat >> intstat_riscstatshift) & 0xf;
		vstat &= bt848->intmask;

		if (bt878)
			astat = bt878->intstat & bt878->intmask;
		else
			astat = 0;

		if (vstat == 0 && astat == 0)
			break;

		if (astat)
			print("vstat %.8luX, astat %.8luX\n", vstat, astat);

		bt848->intstat = vstat;
		if (bt878)
			bt878->intstat = astat;

		if ((vstat & intstat_fmtchg) == intstat_fmtchg) {
			iprint("int: fmtchg\n");
			vstat &= ~intstat_fmtchg;
		}

		if ((vstat & intstat_vpress) == intstat_vpress) {
//			iprint("int: vpress\n");
			vstat &= ~intstat_vpress;
		}

		if ((vstat & intstat_vsync) == intstat_vsync)
			vstat &= ~intstat_vsync;

		if ((vstat & intstat_scerr) == intstat_scerr) {
			iprint("int: scerr\n");
			bt848->gpiodmactl &=
				~(gpiodmactl_riscenable|gpiodmactl_fifoenable);
			bt848->gpiodmactl |= gpiodmactl_fifoenable;
			bt848->gpiodmactl |= gpiodmactl_riscenable;
			vstat &= ~intstat_scerr;
		}

		if ((vstat & intstat_risci) == intstat_risci) {
			tv->lvframe = fnum;
			vstat &= ~intstat_risci;
		}

		if ((vstat & intstat_ocerr) == intstat_ocerr) {
			iprint("int: ocerr\n");
			vstat &= ~intstat_ocerr;
		}

		if ((vstat & intstat_fbus) == intstat_fbus) {
			iprint("int: fbus\n");
			vstat &= ~intstat_fbus;
		}

		if (vstat)
			iprint("int: (v) ignored interrupts %.8ulX\n", vstat);

		if ((astat & intstat_risci) == intstat_risci) {
			tv->narblocks++;
			if ((tv->narblocks % 100) == 0)
				print("a");
			wakeup(tv);
			astat &= ~intstat_risci;
		}

		if ((astat & intstat_fdsr) == intstat_fdsr) {
			iprint("int: (a) fdsr\n");
			bt848->gpiodmactl &=
				~(gpiodmactl_acapenable |
					gpiodmactl_riscenable | gpiodmactl_fifoenable);
			astat &= ~intstat_fdsr;
		}

		if (astat)
			iprint("int: (a) ignored interrupts %.8ulX\n", astat);
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
i2cwrite(Tv *tv, uchar addr, uchar sub, uchar data, int both)
{
	Bt848 *bt848 = tv->bt848;
	ulong intstat, d;
	int i;

	bt848->intstat	= intstat_i2cdone;
	d = (addr << 24) | (sub << 16) | tv->i2ccmd;
	if (both)
		d |= (data << 8) | i2c_bt848w3b;
	bt848->i2c = d;

	intstat = 0;
	for (i = 0; i != 1000; i++) {
		if ((intstat = bt848->intstat) & intstat_i2cdone)
			break;
		microdelay(1000);
	}

	if (i == i2c_timeout) {
		print("i2cwrite: timeout\n");
		return 0;
	}

	if ((intstat & intstat_i2crack) == 0)
		return 0;

	return 1;
}

static ulong *
riscpacked(ulong pa, int fnum, int w, int h, int stride, ulong **lastjmp)
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
		*p++ = pa + i * 2 * stride;
	}

	*p++ = riscsync | riscsync_resync | riscsync_vro;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm1;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite | w | riscwrite_sol | riscwrite_eol;
		*p++ = pa + (i * 2 + 1) * stride;
	}

	/* reset status.  you really need two instructions ;-(. */
	*p++ = riscjmp | (0xf << risclabelshift_reset);
	*p++ = PADDR(p);
	*p++ = riscjmp | riscirq | (fnum << risclabelshift_set);
	*lastjmp = p;

	return pbase;
}

static ulong *
riscplanar411(ulong pa, int fnum, int w, int h, ulong **lastjmp)
{
	ulong *p, *pbase, Cw, Yw, Ch;
	uchar *Ybase, *Cbbase, *Crbase;
	int i, bitspp;

	bitspp = 6;
	assert(w * bitspp / 8 <= 0x7FF);
	pbase = p = (ulong *)malloc((h + 6) * 5 * sizeof(ulong));
	assert(p);

	Yw = w;
	Ybase = (uchar *)pa;
	Cw = w >> 1;
	Ch = h >> 1;
	Cbbase = Ybase + Yw * h;
	Crbase = Cbbase + Cw * Ch;

	*p++ = riscsync | riscsync_resync | riscsync_vre;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm3;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite123 | Yw | riscwrite_sol | riscwrite_eol;
		*p++ = (Cw << 16) | Cw;
		*p++ = (ulong)(Ybase + i * 2 * Yw);
		*p++ = (ulong)(Cbbase + i * Cw);	/* Do not interlace */
		*p++ = (ulong)(Crbase + i * Cw);
	}

	*p++ = riscsync | riscsync_resync | riscsync_vro;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm3;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite1s23 | Yw | riscwrite_sol | riscwrite_eol;
		*p++ = (Cw << 16) | Cw;
		*p++ = (ulong)(Ybase + (i * 2 + 1) * Yw);
	}

	/* reset status.  you really need two instructions ;-(. */
	*p++ = riscjmp | (0xf << risclabelshift_reset);
	*p++ = PADDR(p);
	*p++ = riscjmp | riscirq | (fnum << risclabelshift_set);
	*lastjmp = p;

	return pbase;
}

static ulong *
riscplanar422(ulong pa, int fnum, int w, int h, ulong **lastjmp)
{
	ulong *p, *pbase, Cw, Yw;
	uchar *Ybase, *Cbbase, *Crbase;
	int i, bpp;

	bpp = 2;
	assert(w * bpp <= 0x7FF);
	pbase = p = (ulong *)malloc((h + 6) * 5 * sizeof(ulong));
	assert(p);

	Yw = w;
	Ybase = (uchar *)pa;
	Cw = w >> 1;
	Cbbase = Ybase + Yw * h;
	Crbase = Cbbase + Cw * h;

	*p++ = riscsync | riscsync_resync | riscsync_vre;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm3;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite123 | Yw | riscwrite_sol | riscwrite_eol;
		*p++ = (Cw << 16) | Cw;
		*p++ = (ulong)(Ybase + i * 2 * Yw);
		*p++ = (ulong)(Cbbase + i * 2 * Cw);
		*p++ = (ulong)(Crbase + i * 2 * Cw);
	}

	*p++ = riscsync | riscsync_resync | riscsync_vro;
	*p++ = 0;

	*p++ = riscsync | riscsync_fm3;
	*p++ = 0;

	for (i = 0; i != h / 2; i++) {
		*p++ = riscwrite123 | Yw | riscwrite_sol | riscwrite_eol;
		*p++ = (Cw << 16) | Cw;
		*p++ = (ulong)(Ybase + (i * 2 + 1) * Yw);
		*p++ = (ulong)(Cbbase + (i * 2 + 1) * Cw);
		*p++ = (ulong)(Crbase + (i * 2 + 1) * Cw);
	}

	/* reset status.  you really need two instructions ;-(. */
	*p++ = riscjmp | (0xf << risclabelshift_reset);
	*p++ = PADDR(p);
	*p++ = riscjmp | riscirq | (fnum << risclabelshift_set);
	*lastjmp = p;

	return pbase;
}

static ulong *
riscaudio(ulong pa, int nblocks, int bsize)
{
	ulong *p, *pbase;
	int i;

	pbase = p = (ulong *)malloc((nblocks + 3) * 2 * sizeof(ulong));
	assert(p);

	*p++ = riscsync|riscsync_fm1;
	*p++ = 0;

	for (i = 0; i != nblocks; i++) {
		*p++ = riscwrite | riscwrite_sol | riscwrite_eol | bsize | riscirq |
			((i & 0xf) << risclabelshift_set) |
			((~i & 0xf) << risclabelshift_reset);
		*p++ = pa + i * bsize;
	}

	*p++ = riscsync | riscsync_vro;
	*p++ = 0;
	*p++ = riscjmp;
	*p++ = PADDR(pbase);
	USED(p);

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
	int bitspp, i, bpf;

	if (nframes >= 0x10)
		error(Ebadarg);

	bitspp = getbitspp(tv);
	bpf = w * h * bitspp / 8;

	/* Add one as a spare. */
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

		switch (tv->cfmt) {
		case colorfmt_YCbCr422:
			frames[i].fstart = riscplanar422(PADDR(frames[i].fbase),				i, w, h, &frames[i].fjmp);
			break;
		case colorfmt_YCbCr411:
			frames[i].fstart = riscplanar411(PADDR(frames[i].fbase),
				i, w, h, &frames[i].fjmp);
			break;
		case colorfmt_rgb16:
			frames[i].fstart = riscpacked(PADDR(frames[i].fbase), i,
				w * bitspp / 8, h, stride * bitspp / 8,
				&frames[i].fjmp);
			break;
		default:
			panic("vstart: Unsupport colorformat\n");
		}
	}

	for (i = 0; i != nframes; i++)
		*frames[i].fjmp = PADDR(i == nframes - 1? frames[0].fstart:
			frames[i + 1].fstart);

	vactivate(tv, frames, nframes);
}

static void
astart(Tv *tv, char *input, uint rate, uint nab, uint nasz)
{
	Bt848 *bt878 = tv->bt878;
	ulong *arisc;
	int selector;
	uchar *abuf;
	int s, d;

	if (bt878 == nil || tv->amux == nil)
		error("#V: Card does not support audio");

	selector = 0;
	if (!strcmp(input, "tv"))
		selector = asel_tv;
	else if (!strcmp(input, "radio"))
		selector = asel_radio;
	else if (!strcmp(input, "mic"))
		selector = asel_mic;
	else if (!strcmp(input, "smxc"))
		selector = asel_smxc;
	else
		error("#V: Invalid input");

	if (nasz > 0xfff)
		error("#V: Audio block size too big (max 0xfff)");

	abuf = (uchar *)malloc(nab * nasz * sizeof(uchar));
	assert(abuf);
	arisc = riscaudio(PADDR(abuf), nab, nasz);

	ilock(tv);
	if (tv->arisc) {
		iunlock(tv);
		free(abuf);
		free(arisc);
		error(Einuse);
	}

	tv->arisc = arisc;
	tv->abuf = abuf;
	tv->nablocks = nab;
	tv->absize = nasz;

	bt878->riscstrtadd = PADDR(tv->arisc);
	bt878->packetlen = (nab << 16) | nasz;
	bt878->intmask = intstat_scerr | intstat_ocerr | intstat_risci |
			intstat_pabort | intstat_riperr | intstat_pperr |
			intstat_fdsr | intstat_ftrgt | intstat_fbus;

	/* Assume analog, 16bpp */
	for (s = 0; s < 16; s++)
		if (rate << s > Hwbase_ad * 4 / 15)
			break;
	for (d = 15; d >= 4; d--)
		if (rate << s < Hwbase_ad * 4 / d)
			break;

	print("astart: sampleshift %d, decimation %d\n", s, d);

	tv->narblocks = 0;
	bt878->gpiodmactl = gpiodmactl_fifoenable |
		gpiodmactl_riscenable | gpiodmactl_acapenable |
		gpiodmactl_daes2 |		/* gpiodmactl_apwrdn | */
		gpiodmactl_daiomda | d << 8 | 9 << 28 | selector << 24;
	print("dmactl %.8ulX\n", bt878->gpiodmactl);
	iunlock(tv);
}

static void
astop(Tv *tv)
{
	Bt848 *bt878 = tv->bt878;

	ilock(tv);
	if (tv->aref.ref > 0) {
		iunlock(tv);
		error(Einuse);
	}

	if (tv->abuf) {
		bt878->gpiodmactl &= ~gpiodmactl_riscenable;
		bt878->gpiodmactl &= ~gpiodmactl_fifoenable;

		free(tv->abuf);
		tv->abuf = nil;
		free(tv->arisc);
		tv->arisc = nil;
	}
	iunlock(tv);
}

static void
vgastart(Tv *tv, ulong pa, int stride)
{
	Frame *frame;

	frame = (Frame *)malloc(sizeof(Frame));
	assert(frame);
	if (waserror()) {
		free(frame);
		nexterror();
	}

	frame->fbase = nil;
	frame->fstart = riscpacked(pa, 0, ntsc_hactive * getbitspp(tv) / 8,
		ntsc_vactive, stride * getbitspp(tv) / 8, &frame->fjmp);
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

static long hrcfreq[] = {		/* HRC CATV frequencies */
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
		cfg = tuner->VHF_H;
	else
		cfg = tuner->UHF;

	div = (freq + tuner->offs + finetune) & 0x7fff;

	if (!i2cwrite(tv, tv->i2ctuneraddr, (div >> 8) & 0x7f, div, 1))
		error(Eio);

	if (!i2cwrite(tv, tv->i2ctuneraddr, tuner->cfg, cfg, 1))
		error(Eio);

	tv->channel = channel;
	if (tv->msp)
		msptune(tv);
}

static struct {
	char	*cmode;
	ulong	realmode;
	ulong	cbits;
} colormodes[] = {
	{ "RGB16",	colorfmt_rgb16,		colorfmt_rgb16, },
	{ "YCbCr422",	colorfmt_YCbCr422,	colorfmt_YCbCr422, },
	{ "YCbCr411",	colorfmt_YCbCr411,	colorfmt_YCbCr422, },
};

static void
colormode(Tv *tv, char *colormode)
{
	Bt848 *bt848 = tv->bt848;
	int i;

	for (i = 0; i != nelem(colormodes); i++)
		if (!strcmp(colormodes[i].cmode, colormode))
			break;

	if (i == nelem(colormodes))
		error(Ebadarg);

	tv->cfmt = colormodes[i].realmode;
	bt848->colorfmt = colormodes[i].cbits;
}

static int
getbitspp(Tv *tv)
{
	switch (tv->cfmt) {
	case colorfmt_rgb16:
	case colorfmt_YCbCr422:
		return 16;
	case colorfmt_YCbCr411:
		return 12;
	default:
		error("getbitspp: Unsupport color format\n");
	}
	return -1;
}

static char *
getcolormode(ulong cmode)
{
	switch (cmode) {
	case colorfmt_rgb16:
		return "RGB16";
	case colorfmt_YCbCr411:
		return "YCbCr411";
	case colorfmt_YCbCr422:
		return (cmode == colorfmt_YCbCr422)? "YCbCr422": "YCbCr411";
	default:
		error("getcolormode: Unsupport color format\n");
	}
	return nil;
}

static void
i2c_set(Tv *tv, int scl, int sda)
{
	Bt848 *bt848 = tv->bt848;
	ulong d;

	bt848->i2c = (scl << 1) | sda;
	d = bt848->i2c;
	USED(d);
	microdelay(i2c_delay);
}

static uchar
i2c_getsda(Tv *tv)
{
	Bt848 *bt848 = tv->bt848;

	return bt848->i2c & i2c_sda;
}

static void
i2c_start(Tv *tv)
{
	i2c_set(tv, 0, 1);
	i2c_set(tv, 1, 1);
	i2c_set(tv, 1, 0);
	i2c_set(tv, 0, 0);
}

static void
i2c_stop(Tv *tv)
{
	i2c_set(tv, 0, 0);
	i2c_set(tv, 1, 0);
	i2c_set(tv, 1, 1);
}

static void
i2c_bit(Tv *tv, int sda)
{
	i2c_set(tv, 0, sda);
	i2c_set(tv, 1, sda);
	i2c_set(tv, 0, sda);
}

static int
i2c_getack(Tv *tv)
{
	int ack;

	i2c_set(tv, 0, 1);
	i2c_set(tv, 1, 1);
	ack = i2c_getsda(tv);
	i2c_set(tv, 0, 1);
	return ack;
}

static int
i2c_wr8(Tv *tv, uchar d, int wait)
{
	int i, ack;

	i2c_set(tv, 0, 0);
	for (i = 0; i != 8; i++) {
		i2c_bit(tv, (d & 0x80)? 1: 0);
		d <<= 1;
	}
	if (wait)
		microdelay(wait);

	ack = i2c_getack(tv);
	return ack == 0;
}

static uchar
i2c_rd8(Tv *tv, int lastbyte)
{
	int i;
	uchar d;

	d = 0;
	i2c_set(tv, 0, 1);
	for (i = 0; i != 8; i++) {
		i2c_set(tv, 1, 1);
		d <<= 1;
		if (i2c_getsda(tv))
			d |= 1;
		i2c_set(tv, 0, 1);
	}

	i2c_bit(tv, lastbyte? 1: 0);
	return d;
}

static int
mspsend(Tv *tv, uchar *cmd, int ncmd)
{
	int i, j, delay;

	for (i = 0; i != 3; i++) {
		delay = 2000;

		i2c_start(tv);
		for (j = 0; j != ncmd; j++) {
			if (!i2c_wr8(tv, cmd[j], delay))
				break;
			delay = 0;
		}
		i2c_stop(tv);

		if (j == ncmd)
			return 1;

		microdelay(10000);
		print("mspsend: retrying\n");
	}

	return 0;
}

static int
mspwrite(Tv *tv, uchar sub, ushort reg, ushort v)
{
	uchar b[6];

	b[0] = i2c_msp3400;
	b[1] = sub;
	b[2] = reg >> 8;
	b[3] = reg;
	b[4] = v >> 8;
	b[5] = v;
	return mspsend(tv, b, sizeof b);
}

static int
mspread(Tv *tv, uchar sub, ushort reg, ushort *data)
{
	uchar b[4];
	int i;

	b[0] = i2c_msp3400;
	b[1] = sub;
	b[2] = reg >> 8;
	b[3] = reg;

	for (i = 0; i != 3; i++) {
		i2c_start(tv);
		if (!i2c_wr8(tv, b[0], 2000) ||
			!i2c_wr8(tv, b[1] | 1, 0) ||
			!i2c_wr8(tv, b[2], 0) ||
			!i2c_wr8(tv, b[3], 0)) {

			i2c_stop(tv);
			microdelay(10000);
			print("retrying\n");
			continue;
		}

		i2c_start(tv);

		if (!i2c_wr8(tv, b[0] | 1, 2000)) {
			i2c_stop(tv);
			continue;
		}

		*data  = i2c_rd8(tv, 0) << 8;
		*data |= i2c_rd8(tv, 1);
		i2c_stop(tv);
		return 1;
	}
	return 0;
}

static uchar mspt_reset[] = { i2c_msp3400, 0, 0x80, 0 };
static uchar mspt_on[] = { i2c_msp3400, 0, 0, 0 };

static int
mspreset(Tv *tv)
{
	ushort v, p;
	Bt848 *bt848 = tv->bt848;
	ulong b;

	b = 1 << 5;
	gpioenable(tv, ~b, b);
	gpiowrite(tv, ~b, 0);
	microdelay(2500);
	gpiowrite(tv, ~b, b);

	bt848->i2c = 0x80;

	microdelay(2000);
	mspsend(tv, mspt_reset, sizeof mspt_reset);

	microdelay(2000);
	if (!mspsend(tv, mspt_on, sizeof mspt_on)) {
		print("#V: Cannot find MSP34x5G on the I2C bus (on)\n");
		return 0;
	}
	microdelay(2000);

	if (!mspread(tv, msp_bbp, 0x001e, &v)) {
		print("#V: Cannot read MSP34xG5 chip version\n");
		return 0;
	}

	if (!mspread(tv, msp_bbp, 0x001f, &p)) {
		print("#V: Cannot read MSP34xG5 product code\n");
		return 0;
	}

	print("#V: MSP34%dg ROM %.d, %d.%d\n",
		(uchar)(p >> 8), (uchar)p, (uchar)(v >> 8), (uchar)v);

	tv->msp = 1;
	return 1;
}

static void
mspvolume(Tv *tv, int mute, int l, int r)
{
	short v, d;
	ushort b;

	if (mute) {
		v = 0;
		b = 0;
	}
	else {
		tv->aleft = l;
		tv->aright = r;
		d = v = max(l, r);
		if (d == 0)
			d++;
		b = ((r - l) * 0x7f) / d;
	}

	mspwrite(tv, msp_bbp, 0, v << 8);
	mspwrite(tv, msp_bbp, 7, v? 0x4000: 0);
	mspwrite(tv, msp_bbp, 1, b << 8);
}

static char *
mspaformat(int f)
{
	switch (f) {
	case 0:
		return "unknown";
	case 2:
	case 0x20:
	case 0x30:
		return "M-BTSC";
	case 3:
		return "B/G-FM";
	case 4:
	case 9:
	case 0xB:
		return "L-AM/NICAM D/Kn";
	case 8:
		return "B/G-NICAM";
	case 0xA:
		return "I";
	case 0x40:
		return "FM-Radio";
	}
	return "unknown format";
}


static void
msptune(Tv *tv)
{
	ushort d, s, nicam;
	int i;

	mspvolume(tv, 1, 0, 0);
	if (!mspwrite(tv, msp_dem, 0x0030, 0x2033))
		error("#V: Cannot set MODUS register");

	if (!mspwrite(tv, msp_bbp, 0x0008, 0x0320))
		error("#V: Cannot set loadspeaker input");

	if (!mspwrite(tv, msp_dem, 0x0040, 0x0001))
		error("#V: Cannot set I2S clock freq");
	if (!mspwrite(tv, msp_bbp, 0x000d, 0x1900))
		error("#V: Cannot set SCART prescale");
	if (!mspwrite(tv, msp_bbp, 0x000e, 0x2403))
		error("#V: Cannot set FM/AM prescale");
	if (!mspwrite(tv, msp_bbp, 0x0010, 0x5a00))
		error("#V: Cannot set NICAM prescale");
	if (!mspwrite(tv, msp_dem, 0x0020, 0x0001))
		error("#V: Cannot start auto detect");

	for (d = (ushort)-1, i = 0; i != 10; i++) {
		if (!mspread(tv, msp_dem, 0x007e, &d))
			error("#V: Cannot get autodetect info MSP34xG5");

		if (d == 0 || d < 0x800)
			break;
		delay(50);
	}

	if (!mspread(tv, msp_dem, 0x0200, &s))
		error("#V: Cannot get status info MSP34xG5");

	mspvolume(tv, 0, tv->aleft, tv->aright);

	nicam = ((s >> 4) & 2) | ((s >> 9) & 1);
	snprint(tv->ainfo, sizeof tv->ainfo, "%s %s %s",
		mspaformat(d), (s & (1 << 6))? "stereo": "mono",
		nicamstate[nicam]);
}

static void
i2cscan(Tv *tv)
{
	int i, ack;

	for (i = 0; i < 0x100; i += 2) {
		i2c_start(tv);
		ack = i2c_wr8(tv, i, 0);
		i2c_stop(tv);
		if (ack)
			print("i2c device @%.2uX\n", i);
	}

	for (i = 0xf0; i != 0xff; i++) {
		i2c_start(tv);
		ack = i2c_wr8(tv, i, 0);
		i2c_stop(tv);
		if (ack)
			print("i2c device may be at @%.2uX\n", i);
	}
}

static void
gpioenable(Tv *tv, ulong mask, ulong data)
{
	Bt848 *bt848 = tv->bt848;

	bt848->gpioouten = (bt848->gpioouten & mask) | data;
}

static void
gpiowrite(Tv *tv, ulong mask, ulong data)
{
	Bt848 *bt848 = tv->bt848;

	bt848->gpiodata[0] = (bt848->gpiodata[0] & mask) | data;
}

static void
alteraoutput(Tv *tv)
{
	if (tv->gpiostate == Gpiooutput)
		return;

	gpioenable(tv, ~0xffffff, 0x56ffff);
	microdelay(10);
	tv->gpiostate = Gpiooutput;
}

static void
alterainput(Tv *tv)
{
	if (tv->gpiostate == Gpioinput)
		return;

	gpioenable(tv, ~0xffffff, 0x570000);
	microdelay(10);
	tv->gpiostate = Gpioinput;
}

static void
alterareg(Tv *tv, ulong reg)
{
	if (tv->alterareg == reg)
		return;

	gpiowrite(tv, ~0x56ffff, (reg & 0x54ffff) | tv->alteraclock);
	microdelay(10);
	tv->alterareg = reg;
}

static void
alterawrite(Tv *tv, ulong reg, ushort data)
{
	alteraoutput(tv);
	alterareg(tv, reg);

	tv->alteraclock ^= 0x20000;
	gpiowrite(tv, ~0x56ffff, (reg & 0x540000) | data | tv->alteraclock);
	microdelay(10);
}

static void
alteraread(Tv *tv, int reg, ushort *data)
{
	Bt848 *bt848 = tv->bt848;

	if (tv->alterareg != reg) {
		alteraoutput(tv);
		alterareg(tv, reg);
	}
	else {
		gpioenable(tv, ~0xffffff, 0x560000);
		microdelay(10);
	}

	alterainput(tv);
	gpiowrite(tv, ~0x570000, (reg & 0x560000) | tv->alteraclock);
	microdelay(10);
	*data = (ushort)bt848->gpiodata[0];
	microdelay(10);
}

static void
kfirloadu(Tv *tv, uchar *u, int ulen)
{
	Bt848 *bt848 = tv->bt848;
	int i, j;

	ilock(&tv->kfirlock);
	bt848->gpioouten &= 0xff000000;
	bt848->gpioouten |= gpio_altera_data |
		gpio_altera_clock | gpio_altera_nconfig;
	bt848->gpiodata[0] &= 0xff000000;
	microdelay(10);
	bt848->gpiodata[0] |= gpio_altera_nconfig;
	microdelay(10);

	/* Download the microcode */
	for (i = 0; i != ulen; i++)
		for (j = 0; j != 8; j++) {
			bt848->gpiodata[0] &= ~(gpio_altera_clock|gpio_altera_data);
			if (u[i] & 1)
				bt848->gpiodata[0] |= gpio_altera_data;
			bt848->gpiodata[0] |= gpio_altera_clock;
			u[i] >>= 1;
		}
	bt848->gpiodata[0] &= ~gpio_altera_clock;
	microdelay(100);

	/* Initialize. */
	for (i = 0; i != 30; i++) {
		bt848->gpiodata[0] &= ~gpio_altera_clock;
		bt848->gpiodata[0] |= gpio_altera_clock;
	}
	bt848->gpiodata[0] &= ~(gpio_altera_clock|gpio_altera_data);
	iunlock(&tv->kfirlock);

	tv->gpiostate = Gpioinit;
}

static void
kfirreset(Tv *tv)
{
	alterawrite(tv, 0, 0);
	microdelay(10);
	alterawrite(tv, 0x40000, 0);
	microdelay(10);
	alterawrite(tv, 0x40006, 0x80);
	microdelay(10);
	alterawrite(tv, 8, 1);
	microdelay(10);
	alterawrite(tv, 0x40004, 2);
	microdelay(10);
	alterawrite(tv, 4, 3);
	microdelay(3);
}

static int
kfirinitialize(Tv *tv)
{
	/* Initialize parameters? */

	tv->gpiostate = Gpioinit;
	tv->alterareg = -1;
	tv->alteraclock = 0x20000;
	kfirloadu(tv, hcwAMC, sizeof hcwAMC);
	kfirreset(tv);
	return 1;
}
