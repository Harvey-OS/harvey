#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"
#include "radeon.h"

static int	debug = 0;

#define DBGPRINT	if (debug) print

enum {
	Kilo	 = 1024,
	Mega	 = Kilo *Kilo,
};

enum {
	DISPLAY_CRT,
	DISPLAY_FP,
	DISPLAY_LCD,
};

typedef struct Radeon Radeon;
struct Radeon {
	uintptr	mmio;
	Pcidev	*pci;
	uchar	*bios;

	ulong	fbsize;
	int	display_type;

	ulong	ovr_clr;
	ulong	ovr_wid_top_bottom;
	ulong	ovr_wid_left_right;
	ulong	ov0_scale_cntl;
	ulong	subpic_cntl;
	ulong	viph_control;
	ulong	i2c_cntl_1;
	ulong	rbbm_soft_reset;
	ulong	cap0_trig_cntl;
	ulong	cap1_trig_cntl;
	ulong	gen_int_cntl;
	ulong	bus_cntl;

	ulong	crtc_gen_cntl;
	ulong	crtc_ext_cntl;
	ulong	dac_cntl;

	ulong	crtc_h_total_disp;
	ulong	crtc_h_sync_strt_wid;
	ulong	crtc_v_total_disp;
	ulong	crtc_v_sync_strt_wid;

	ulong	crtc_pitch;

	ulong	crtc_offset;
	ulong	crtc_offset_cntl;

	ulong	htotal_cntl;

	ulong	surface_cntl;

	int	r300_workaround;

	/* inited from rom */
	ushort	reference_freq;
	ushort	reference_div;
	ushort	xclk;
	ulong	max_pll_freq;
	ulong	min_pll_freq;

	ulong	pll_output_freq;
	ulong	feedback_div;
	ulong	dot_clock_freq;

	ulong	post_div;
	ulong	ppll_ref_div;
	ulong	ppll_div_3;
};

/* from io.c */
extern char *readbios(long len, long offset);

static void radeon300_workaround(Radeon*radeon);

static void
OUTREG8(Radeon*radeon, ulong offset, uchar val)
{
	((uchar *)(radeon->mmio + offset))[0] = val;
}

static void
OUTREG(Radeon*radeon, ulong offset, ulong val)
{
	((ulong *)(radeon->mmio + offset))[0] = val;
}

static ulong
INREG(Radeon*radeon, ulong offset)
{
	return ((ulong *)(radeon->mmio + offset))[0];
}

static void
OUTREGP(Radeon*radeon, ulong offset, ulong val, ulong mask)
{
	OUTREG(radeon, offset, (INREG(radeon, offset) & mask) | val);
}

static void
OUTPLL(Radeon*radeon, ulong offset, ulong val)
{
	OUTREG8(radeon, CLOCK_CNTL_INDEX,
		(offset & 0x3f) | PLL_WR_EN);
	OUTREG(radeon, CLOCK_CNTL_DATA, val);
}

static ulong
INPLL(Radeon*radeon, ulong offset)
{
	ulong data;

	OUTREG8(radeon, CLOCK_CNTL_INDEX, offset & 0x3f);
	data = INREG(radeon, CLOCK_CNTL_DATA);
	if (radeon->r300_workaround)
		radeon300_workaround(radeon);
	return data;
}

static void
OUTPLLP(Radeon*radeon, ulong offset, ulong val, ulong mask)
{
	OUTPLL(radeon, offset, (INPLL(radeon, offset) & mask) | val);
}

static void
radeon300_workaround(Radeon*radeon)
{
	ulong save, tmp;

	save = INREG(radeon, CLOCK_CNTL_INDEX);
	tmp = save & ~(0x3f | PLL_WR_EN);
	OUTREG(radeon, CLOCK_CNTL_INDEX, tmp);
	tmp = INREG(radeon, CLOCK_CNTL_DATA);
	OUTREG(radeon, CLOCK_CNTL_INDEX, save);
	USED(tmp);
}

static void
radeon_getbiosparams(Radeon*radeon)
{
	ulong addr;
	ushort offset, pib;
	uchar *bios;

	radeon->bios = nil;
	addr = 0xC0000;
	bios = (uchar *)readbios(0x10000, addr);
	if (bios[0] != 0x55 || bios[1] != 0xAA) {
		addr = 0xE0000;
		bios = (uchar *)readbios(0x10000, addr);
		if (bios[0] != 0x55 || bios[1] != 0xAA) {
			print("radeon: bios not found\n");
			return;
		}
	}

	radeon->bios = bios;
	offset = BIOS16(radeon, BIOS_START);

	pib = BIOS16(radeon, offset + 0x30);

	radeon->reference_freq	 = BIOS16(radeon, pib + 0x0e);
	radeon->reference_div	 = BIOS16(radeon, pib + 0x10);
	radeon->min_pll_freq	 = BIOS32(radeon, pib + 0x12);
	radeon->max_pll_freq	 = BIOS32(radeon, pib + 0x16);
	radeon->xclk		 = BIOS16(radeon, pib + 0x08);

	DBGPRINT("radeon: bios=0x%08ulx offset=0x%ux\n", addr, offset);
	DBGPRINT("radeon: pll_info_block: 0x%ux\n", pib);
	DBGPRINT("radeon: reference_freq: %ud\n", radeon->reference_freq);
	DBGPRINT("radeon: reference_div:  %ud\n", radeon->reference_div);
	DBGPRINT("radeon: min_pll_freq:   %uld\n", radeon->min_pll_freq);
	DBGPRINT("radeon: max_pll_freq:   %uld\n", radeon->max_pll_freq);
	DBGPRINT("radeon: xclk:           %ud\n", radeon->xclk);
}

static Pcidev *
radeonpci(int *isr300)
{
	static Pcidev * p = nil;
	struct pciids *ids;

	DBGPRINT("radeon: ATI Technologies Inc. Radeon [789]xxx drivers (v0.1)\n");
	while ((p = pcimatch(p, ATI_PCIVID, 0)) != nil)
		for (ids = radeon_pciids; ids->did; ids++)
			if (ids->did == p->did) {
				DBGPRINT("radeon: Found %s\n", ids->name);
				DBGPRINT("radeon: did:%04ux rid:%02ux\n",
					p->did, p->rid);
				if (isr300)
					*isr300 = ids->type == ATI_R300;
				return p;
			}
	DBGPRINT("radeon: not found!\n");
	return nil;
}

static void
vga_disable(Vga*vga)
{
	Ctlr *c;

	for (c = vga->link; c; c = c->link)
		if (strncmp(c->name, "vga", 3) == 0)
			c->load = nil;
}

static void
snarf(Vga *vga, Ctlr *ctlr)
{
	int isr300;
	ulong tmp;
	uintptr mmio;
	Pcidev *p;
	Radeon *radeon;

	if (vga->private == nil) {
		vga_disable(vga);

		vga->private = alloc(sizeof(Radeon));
		radeon = vga->private;

		p = radeonpci(&isr300);
		if (p == nil)
			error("%s: not found\n", ctlr->name);

		vgactlw("type", ctlr->name);

		mmio = (uintptr)segattach(0, "radeonmmio", (void *)0,
			p->mem[2].size);
		if (mmio == ~0)
			error("%s: can't attach mmio segment\n", ctlr->name);

		DBGPRINT("radeon: mmio address: %08#p [size=%#x]\n",
			(void *)mmio, p->mem[2].size);

		radeon->pci = p;
		radeon->r300_workaround = isr300;
		radeon->mmio = mmio;
	}

	radeon = vga->private;
	radeon->fbsize = INREG(radeon, CONFIG_MEMSIZE);
	vga->vmz = radeon->fbsize;
	DBGPRINT("radeon: frame buffer size=%uld [%uldMB]\n",
		radeon->fbsize, radeon->fbsize / Mega);

	tmp = INREG(radeon, FP_GEN_CNTL);
	if (tmp & FP_EN_TMDS)
		radeon->display_type = DISPLAY_FP;
	else
		radeon->display_type = DISPLAY_CRT;

	DBGPRINT("radeon: display type: %s\n",
		radeon->display_type == DISPLAY_CRT? "CRT": "FLAT PANEL");

	if (radeon->display_type != DISPLAY_CRT)
		error("unsupported NON CRT Display\n");

	radeon_getbiosparams(radeon);
	radeon->bus_cntl = INREG(radeon, BUS_CNTL);
	DBGPRINT("radeon: PPLL_CNTL=0x%08ulx\n", INPLL(radeon, PPLL_CNTL));
	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr*ctlr)
{
	ctlr->flag |= Hlinear | Foptions;
}

static int
radeondiv(int n, int d)
{
	return (n + d/2) / d;
}

static void
radeon_init_common_registers(Radeon*radeon)
{
	radeon->ovr_clr		= 0;
	radeon->ovr_wid_left_right	= 0;
	radeon->ovr_wid_top_bottom	= 0;
	radeon->ov0_scale_cntl	= 0;
	radeon->subpic_cntl	= 0;
	radeon->viph_control	= 0;
	radeon->i2c_cntl_1	= 0;
	radeon->rbbm_soft_reset	= 0;
	radeon->cap0_trig_cntl	= 0;
	radeon->cap1_trig_cntl	= 0;
	if (radeon->bus_cntl & BUS_READ_BURST)
		radeon->bus_cntl |= BUS_RD_DISCARD_EN;
}

static void
radeon_init_crtc_registers(Radeon*radeon, Mode*mode)
{
	int format, dac6bit, hsync_wid, vsync_wid, hsync_start, hsync_fudge;
	int bpp;
	static int hsync_fudge_default[] = {
		0x00, 0x12, 0x09, 0x09, 0x06, 0x05,
	};

	format = 0;
	bpp = 0;
	dac6bit = 0;
	switch (mode->z) {
	case 6:
		format = 2;
		dac6bit = 1;
		bpp =  8;
		break;
	case 8:
		format = 2;
		dac6bit = 0;
		bpp =  8;
		break;
	case 15:
		format = 3;
		dac6bit = 0;
		bpp = 16;
		break;
	case 16:
		format = 4;
		dac6bit = 0;
		bpp = 16;
		break;
	case 24:
		format = 5;
		dac6bit = 0;
		bpp = 24;
		break;
	case 32:
		format = 6;
		dac6bit = 0;
		bpp = 32;
		break;
	default:
		error("radeon: unsupported mode depth %d\n", mode->z);
	}
	hsync_fudge = hsync_fudge_default[format-1];

	DBGPRINT("mode->z = %d (format = %d, bpp = %d, dac6bit = %s)\n",
		mode->z, format, bpp, dac6bit? "true": "false");

	radeon->crtc_gen_cntl = CRTC_EN | CRTC_EXT_DISP_EN | format << 8 |
		(mode->interlace? CRTC_INTERLACE_EN: 0);
	radeon->crtc_ext_cntl = VGA_ATI_LINEAR | XCRT_CNT_EN | CRTC_CRT_ON;
	radeon->dac_cntl = DAC_MASK_ALL | DAC_VGA_ADR_EN |
		(dac6bit? 0: DAC_8BIT_EN);

	radeon->crtc_h_total_disp = ((mode->ht/8 - 1) & 0x3ff) |
		((mode->x/8 - 1) & 0x1ff) << 16;

	hsync_wid = (mode->ehb - mode->shb) / 8;
	if (hsync_wid == 0)
		hsync_wid = 1;

	hsync_start = mode->shb - 8 + hsync_fudge;

	DBGPRINT("hsync_start=%d hsync_wid=%d hsync_fudge=%d\n",
		hsync_start, hsync_wid, hsync_fudge);

	radeon->crtc_h_sync_strt_wid = ((hsync_start & 0x1fff) |
		(hsync_wid & 0x3f) << 16 | (mode->hsync? CRTC_H_SYNC_POL: 0));
	radeon->crtc_v_total_disp = ((mode->vt - 1) & 0xffff) |
		(mode->y - 1) << 16;

	vsync_wid = mode->vre - mode->vrs;
	if (!vsync_wid)
		vsync_wid = 1;

	radeon->crtc_v_sync_strt_wid = (((mode->vrs - 1) & 0xfff) |
		(vsync_wid & 0x1f) << 16 | (mode->vsync? CRTC_V_SYNC_POL: 0));
	radeon->crtc_offset = 0;
	radeon->crtc_offset_cntl = INREG(radeon, CRTC_OFFSET_CNTL);
	radeon->crtc_pitch = (mode->x * bpp + bpp * 8 - 1) / (bpp * 8);
	radeon->crtc_pitch |= radeon->crtc_pitch << 16;
}

struct divider {
	int	divider;
	int	bitvalue;
};

static void
radeon_init_pll_registers(Radeon*radeon, ulong freq)
{
	struct divider *post_div;
	static struct divider post_divs[] = {
		{ 1, 0 },
		{ 2, 1 },
		{ 4, 2 },
		{ 8, 3 },
		{ 3, 4 },
		{ 16, 5 },
		{ 6, 6 },
		{ 12, 7 },
		{ 0, 0 }
	};

	DBGPRINT("radeon: initpll: freq=%uld\n", freq);

	if (freq > radeon->max_pll_freq)
		freq = radeon->max_pll_freq;
	if (freq * 12 < radeon->min_pll_freq)
		freq = radeon->min_pll_freq / 12;

	for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
		radeon->pll_output_freq = post_div->divider * freq;
		if (radeon->pll_output_freq >= radeon->min_pll_freq &&
		    radeon->pll_output_freq <= radeon->max_pll_freq)
			break;
	}

	radeon->dot_clock_freq = freq;
	radeon->feedback_div = radeondiv(radeon->reference_div *
		radeon->pll_output_freq, radeon->reference_freq);
	radeon->post_div = post_div->divider;

	DBGPRINT("dc=%uld, of=%uld, fd=%uld, pd=%uld\n", radeon->dot_clock_freq,
		radeon->pll_output_freq, radeon->feedback_div, radeon->post_div);

	radeon->ppll_ref_div = radeon->reference_div;
	radeon->ppll_div_3 = radeon->feedback_div | post_div->bitvalue << 16;
	radeon->htotal_cntl = 0;
	radeon->surface_cntl = 0;
}

static void
init(Vga*vga, Ctlr*ctlr)
{
	Radeon *radeon;
	Mode *mode;

	radeon	 = vga->private;
	mode	 = vga->mode;

	DBGPRINT("radeon: monitor type = '%s'\n", mode->type);
	DBGPRINT("radeon: size = '%s'\n", mode->size);
	DBGPRINT("radeon: chan = '%s'\n", mode->chan);
	DBGPRINT("radeon: freq=%d deffreq=%d x=%d y=%d z=%d\n",
		mode->frequency, mode->deffrequency, mode->x, mode->y, mode->z);
	DBGPRINT("radeon: ht=%d shb=%d ehb=%d shs=%d ehs=%d hsync='%c'\n",
		mode->ht, mode->shb, mode->ehb, mode->shs, mode->ehs,
		mode->hsync? mode->hsync: ' ');
	DBGPRINT("radeon: vt=%d vrs=%d vre=%d vsync='%c'\n",
		mode->vt, mode->vrs, mode->vre, mode->vsync? mode->vsync: ' ');

	radeon_init_common_registers(radeon);
	radeon_init_crtc_registers(radeon, mode);
	radeon_init_pll_registers(radeon, mode->frequency / 10000);
	ctlr->flag |= Finit | Ulinear;
}

static void
radeon_blank(Radeon*radeon)
{
	OUTREGP(radeon, CRTC_EXT_CNTL,
		  CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS | CRTC_HSYNC_DIS,
		~(CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS | CRTC_HSYNC_DIS));
}

static void
radeon_unblank(Radeon*radeon)
{
	OUTREGP(radeon, CRTC_EXT_CNTL, CRTC_CRT_ON,
		~(CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS | CRTC_HSYNC_DIS));
}

static void
radeon_load_common_registers(Radeon*radeon)
{
	OUTREG(radeon, OVR_CLR, 		radeon->ovr_clr);
	OUTREG(radeon, OVR_WID_LEFT_RIGHT, 	radeon->ovr_wid_left_right);
	OUTREG(radeon, OVR_WID_TOP_BOTTOM, 	radeon->ovr_wid_top_bottom);
	OUTREG(radeon, OV0_SCALE_CNTL, 		radeon->ov0_scale_cntl);
	OUTREG(radeon, SUBPIC_CNTL, 		radeon->subpic_cntl);
	OUTREG(radeon, VIPH_CONTROL, 		radeon->viph_control);
	OUTREG(radeon, I2C_CNTL_1, 		radeon->i2c_cntl_1);
	OUTREG(radeon, GEN_INT_CNTL, 		radeon->gen_int_cntl);
	OUTREG(radeon, CAP0_TRIG_CNTL, 		radeon->cap0_trig_cntl);
	OUTREG(radeon, CAP1_TRIG_CNTL, 		radeon->cap1_trig_cntl);
	OUTREG(radeon, BUS_CNTL, 		radeon->bus_cntl);
	OUTREG(radeon, SURFACE_CNTL, 		radeon->surface_cntl);
}

static void
radeon_load_crtc_registers(Radeon*radeon)
{
	OUTREG(radeon, CRTC_GEN_CNTL, radeon->crtc_gen_cntl);
	OUTREGP(radeon, CRTC_EXT_CNTL, radeon->crtc_ext_cntl,
		CRTC_VSYNC_DIS | CRTC_HSYNC_DIS | CRTC_DISPLAY_DIS);
	OUTREGP(radeon, DAC_CNTL, radeon->dac_cntl,
		DAC_RANGE_CNTL | DAC_BLANKING);
	OUTREG(radeon, CRTC_H_TOTAL_DISP, 	radeon->crtc_h_total_disp);
	OUTREG(radeon, CRTC_H_SYNC_STRT_WID, 	radeon->crtc_h_sync_strt_wid);
	OUTREG(radeon, CRTC_V_TOTAL_DISP, 	radeon->crtc_v_total_disp);
	OUTREG(radeon, CRTC_V_SYNC_STRT_WID, 	radeon->crtc_v_sync_strt_wid);
	OUTREG(radeon, CRTC_OFFSET, 		radeon->crtc_offset);
	OUTREG(radeon, CRTC_OFFSET_CNTL, 	radeon->crtc_offset_cntl);
	OUTREG(radeon, CRTC_PITCH, 		radeon->crtc_pitch);
}

static void
radeon_pllwaitupd(Radeon*radeon)
{
	int i;

	for (i = 0; i < 10000; i++)
		if (!(INPLL(radeon, PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R))
			break;
}

static void
radeon_pllwriteupd(Radeon*radeon)
{
	while (INPLL(radeon, PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R)
		;
	OUTPLLP(radeon, PPLL_REF_DIV, PPLL_ATOMIC_UPDATE_W, ~PPLL_ATOMIC_UPDATE_W);
}

static void
radeon_load_pll_registers(Radeon*radeon)
{
	OUTPLLP(radeon, VCLK_ECP_CNTL,
		VCLK_SRC_SEL_CPUCLK, ~VCLK_SRC_SEL_MASK);
	OUTPLLP(radeon, PPLL_CNTL,
	      PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN,
	    ~(PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN));
	OUTREGP(radeon, CLOCK_CNTL_INDEX, PLL_DIV_SEL, ~PLL_DIV_SEL);
	if (radeon->r300_workaround) {
		DBGPRINT("r300_workaround\n");
		if (radeon->ppll_ref_div & R300_PPLL_REF_DIV_ACC_MASK)
			/*
			 * When restoring console mode, use saved PPLL_REF_DIV
			 * setting.
			 */
			OUTPLLP(radeon, PPLL_REF_DIV, radeon->ppll_ref_div, 0);
		else
			/* R300 uses ref_div_acc field as real ref divider */
			OUTPLLP(radeon, PPLL_REF_DIV,
			    radeon->ppll_ref_div << R300_PPLL_REF_DIV_ACC_SHIFT,
				~R300_PPLL_REF_DIV_ACC_MASK);
	} else
		OUTPLLP(radeon, PPLL_REF_DIV, radeon->ppll_ref_div,
			~PPLL_REF_DIV_MASK);

	OUTPLLP(radeon, PPLL_DIV_3, radeon->ppll_div_3, ~PPLL_FB3_DIV_MASK);
	OUTPLLP(radeon, PPLL_DIV_3, radeon->ppll_div_3, ~PPLL_POST3_DIV_MASK);

	radeon_pllwriteupd(radeon);
	radeon_pllwaitupd(radeon);

	OUTPLL(radeon, HTOTAL_CNTL, radeon->htotal_cntl);
	OUTPLLP(radeon, PPLL_CNTL, 0,
		~(PPLL_RESET | PPLL_SLEEP | PPLL_ATOMIC_UPDATE_EN |
		  PPLL_VGA_ATOMIC_UPDATE_EN));

	if (debug) {
		Bprint(&stdout, "Wrote: 0x%08ulx 0x%08ulx 0x%08ulx (0x%08ulx)\n",
			radeon->ppll_ref_div, radeon->ppll_div_3,
			radeon->htotal_cntl, INPLL(radeon, PPLL_CNTL));
		Bprint(&stdout, "Wrote: rd=%uld, fd=%uld, pd=%uld\n",
			radeon->ppll_ref_div & PPLL_REF_DIV_MASK,
			radeon->ppll_div_3 & PPLL_FB3_DIV_MASK,
			(radeon->ppll_div_3 & PPLL_POST3_DIV_MASK) >> 16);
	}

	/* Let the clock to lock */
	sleep(5);

	OUTPLLP(radeon, VCLK_ECP_CNTL, VCLK_SRC_SEL_PPLLCLK, ~VCLK_SRC_SEL_MASK);
}

static void
load(Vga*vga, Ctlr*ctlr)
{
	Radeon *radeon;

	radeon = (Radeon *) vga->private;
	radeon_blank(radeon);
	radeon_load_common_registers(radeon);
	radeon_load_crtc_registers(radeon);
	radeon_load_pll_registers(radeon);
	radeon_unblank(radeon);

	/* init palette [gamma] */
	if (vga->mode->z > 8) {
		int i;

		OUTREG(radeon, PALETTE_INDEX, 0);
		for (i = 0; i < 256; i++)
			OUTREG(radeon, PALETTE_DATA, i << 16 | i << 8 | i);
	}

	ctlr->flag |= Fload;
}

static void
dump(Vga*vga, Ctlr*ctlr)
{
	Radeon *radeon;

	USED(ctlr);
	radeon = (Radeon *)vga->private;
	USED(radeon);
}

Ctlr radeon = {
	"radeon",
	snarf,
	options,
	init,
	load,
	dump,
};
Ctlr radeonhwgc = {
	"radeonhwgc",
	0, 					/* snarf */
	0, 					/* options */
	0, 					/* init */
	0, 					/* load */
	0, 					/* dump */
};
