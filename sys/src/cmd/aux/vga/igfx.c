#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

typedef struct Reg Reg;
typedef struct Dpll Dpll;
typedef struct Hdmi Hdmi;
typedef struct Dp Dp;
typedef struct Fdi Fdi;
typedef struct Pfit Pfit;
typedef struct Curs Curs;
typedef struct Plane Plane;
typedef struct Trans Trans;
typedef struct Pipe Pipe;

enum {
	MHz = 1000000,
};

enum {
	TypeG45,
	TypeIVB,		/* Ivy Bridge */
	TypeSNB,		/* Sandy Bridge (unfinished) */
	TypeHSW,		/* Haswell */
};

enum {
	PortVGA	= 0,		/* adpa */
	PortLCD	= 1,		/* lvds */
	PortDPA	= 2,
	PortDPB	= 3,
	PortDPC	= 4,
	PortDPD	= 5,
	PortDPE	= 6,
};

struct Reg {
	u32int	a;		/* address or 0 when invalid */
	u32int	v;		/* value */
};

struct Dpll {
	Reg	ctrl;		/* DPLLx_CTRL */
	Reg	fp0;		/* FPx0 */
	Reg	fp1;		/* FPx1 */
};

struct Trans {
	Reg	dm[2];		/* pipe/trans DATAM */
	Reg	dn[2];		/* pipe/trans DATAN */
	Reg	lm[2];		/* pipe/trans LINKM */
	Reg	ln[2];		/* pipe/trans LINKN */

	Reg	ht;		/* pipe/trans HTOTAL_x */
	Reg	hb;		/* pipe/trans HBLANK_x */
	Reg	hs;		/* pipe/trans HSYNC_x */
	Reg	vt;		/* pipe/trans VTOTAL_x */
	Reg	vb;		/* pipe/trans VBLANK_x */
	Reg	vs;		/* pipe/trans VSYNC_x */
	Reg	vss;		/* pipe/trans VSYNCSHIFT_x */

	Reg	conf;		/* pipe/trans CONF_x */
	Reg	chicken;	/* workarround register */

	Dpll	*dpll;		/* this transcoders dpll */
	Reg	clksel;		/* HSW: PIPE_CLK_SEL_x: transcoder clock select */
};

struct Hdmi {
	Reg	ctl;
	Reg	bufctl[4];
};

struct Dp {
	/* HSW */
	int	hdmi;
	Reg	stat;
	Reg	bufctl;		/* DDI_BUF_CTL_x */
	Reg	buftrans[20];	/* DDI_BUF_TRANS */

	Reg	ctl;		/* HSW: DP_TP_CTL_x */
	Reg	auxctl;
	Reg	auxdat[5];
};

struct Fdi {
	Trans;

	Reg	txctl;		/* FDI_TX_CTL */

	Reg	rxctl;		/* FDI_RX_CTL */
	Reg	rxmisc;		/* FDI_RX_MISC */
	Reg	rxtu[2];	/* FDI_RX_TUSIZE */

	Reg	dpctl;		/* TRANS_DP_CTL_x */
};

struct Pfit {
	Reg	ctrl;
	Reg	winpos;
	Reg	winsize;
	Reg	pwrgate;
};

struct Plane {
	Reg	cntr;		/* DSPxCNTR */
	Reg	linoff;		/* DSPxLINOFF */
	Reg	stride;		/* DSPxSTRIDE */
	Reg	surf;		/* DSPxSURF */
	Reg	tileoff;	/* DSPxTILEOFF */
	Reg	leftsurf;	/* HSW: PRI_LEFT_SURF_x */

	Reg	pos;
	Reg	size;
};

struct Curs {
	Reg	cntr;
	Reg	base;
	Reg	pos;
};

struct Pipe {
	Trans;			/* cpu transcoder */

	Reg	src;		/* PIPExSRC */

	Fdi	fdi[1];		/* fdi/dp transcoder */

	Plane	dsp[1];		/* display plane */
	Curs	cur[1];		/* hardware cursor */

	Pfit	*pfit;		/* selected panel fitter */
};

typedef struct Igfx Igfx;
struct Igfx {
	Ctlr	*ctlr;
	Pcidev	*pci;

	u32int	pio;
	u32int	*mmio;

	int	type;

	int	npipe;
	Pipe	pipe[4];

	Dpll	dpll[4];
	Pfit	pfit[3];

	/* HSW */
	int	isult;

	Reg	dpllsel[5];	/* DPLL_SEL (IVB), PORT_CLK_SEL_DDIx (HSW) */

	/* IVB */
	Reg	drefctl;	/* DREF_CTL */
	Reg	rawclkfreq;	/* RAWCLK_FREQ */
	Reg	ssc4params;	/* SSC4_PARAMS */

	Dp	dp[5];
	Hdmi	hdmi[4];

	Reg	ppcontrol;
	Reg	ppstatus;

	/* G45 */
	Reg	sdvoc;
	Reg	sdvob;

	/* common */
	Reg	adpa;
	Reg	lvds;

	Reg	vgacntrl;
};

static u32int
rr(Igfx *igfx, u32int a)
{
	if(a == 0)
		return 0;
	assert((a & 3) == 0);
	if(igfx->mmio != nil)
		return igfx->mmio[a/4];
	outportl(igfx->pio, a);
	return inportl(igfx->pio + 4);
}
static void
wr(Igfx *igfx, u32int a, u32int v)
{
	if(a == 0)	/* invalid */
		return;
	assert((a & 3) == 0);
	if(igfx->mmio != nil){
		igfx->mmio[a/4] = v;
		return;
	}
	outportl(igfx->pio, a);
	outportl(igfx->pio + 4, v);
}
static void
csr(Igfx *igfx, u32int reg, u32int clr, u32int set)
{
	wr(igfx, reg, (rr(igfx, reg) & ~clr) | set);
}

static void
loadreg(Igfx *igfx, Reg r)
{
	wr(igfx, r.a, r.v);
}

static Reg
snarfreg(Igfx *igfx, u32int a)
{
	Reg r;

	r.a = a;
	r.v = rr(igfx, a);
	return r;
}

static void
snarftrans(Igfx *igfx, Trans *t, u32int o)
{
	/* pipe timing */
	t->ht	= snarfreg(igfx, o + 0x00000);
	t->hb	= snarfreg(igfx, o + 0x00004);
	t->hs	= snarfreg(igfx, o + 0x00008);
	t->vt	= snarfreg(igfx, o + 0x0000C);
	t->vb	= snarfreg(igfx, o + 0x00010);
	t->vs	= snarfreg(igfx, o + 0x00014);
	t->vss	= snarfreg(igfx, o + 0x00028);

	t->conf	= snarfreg(igfx, o + 0x10008);

	switch(igfx->type){
	case TypeG45:
		if(t == &igfx->pipe[0]){			/* PIPEA */
			t->dm[0] = snarfreg(igfx, 0x70050);	/* GMCHDataM */
			t->dn[0] = snarfreg(igfx, 0x70054);	/* GMCHDataN */
			t->lm[0] = snarfreg(igfx, 0x70060);	/* DPLinkM */
			t->ln[0] = snarfreg(igfx, 0x70064);	/* DPLinkN */
		}
		break;
	case TypeIVB:
		t->dm[0] = snarfreg(igfx, o + 0x30);
		t->dn[0] = snarfreg(igfx, o + 0x34);
		t->dm[1] = snarfreg(igfx, o + 0x38);
		t->dn[1] = snarfreg(igfx, o + 0x3c);
		t->lm[0] = snarfreg(igfx, o + 0x40);
		t->ln[0] = snarfreg(igfx, o + 0x44);
		t->lm[1] = snarfreg(igfx, o + 0x48);
		t->ln[1] = snarfreg(igfx, o + 0x4c);
		break;
	case TypeHSW:
		t->dm[0] = snarfreg(igfx, o + 0x30);
		t->dn[0] = snarfreg(igfx, o + 0x34);
		t->lm[0] = snarfreg(igfx, o + 0x40);
		t->ln[0] = snarfreg(igfx, o + 0x44);
		if(t == &igfx->pipe[3]){			/* eDP pipe */
			t->dm[1] = snarfreg(igfx, o + 0x38);
			t->dn[1] = snarfreg(igfx, o + 0x3C);
			t->lm[1] = snarfreg(igfx, o + 0x48);
			t->ln[1] = snarfreg(igfx, o + 0x4C);
		}
		break;
	}
}

static void
snarfpipe(Igfx *igfx, int x)
{
	u32int o;
	Pipe *p;

	p = &igfx->pipe[x];

	o = 0x60000 + x*0x1000;
	snarftrans(igfx, p, o);

	if(igfx->type != TypeHSW || x != 3)
		p->src = snarfreg(igfx, o + 0x0001C);

	if(igfx->type == TypeHSW) {
		p->dpctl = snarfreg(igfx, o + 0x400);	/* PIPE_DDI_FUNC_CTL_x */
		p->dpll = &igfx->dpll[0];
		if(x == 3){
			x = 0;	/* use plane/cursor a */
			p->src = snarfreg(igfx, 0x6001C);
		}else
			p->clksel = snarfreg(igfx, 0x46140 + x*4);
	} else if(igfx->type == TypeIVB || igfx->type == TypeSNB) {
		p->fdi->txctl = snarfreg(igfx, o + 0x100);

		o = 0xE0000 | x*0x1000;
		snarftrans(igfx, p->fdi, o);

		p->fdi->dpctl = snarfreg(igfx, o + 0x300);

		p->fdi->rxctl = snarfreg(igfx, o + 0x1000c);
		p->fdi->rxmisc = snarfreg(igfx, o + 0x10010);
		p->fdi->rxtu[0] = snarfreg(igfx, o + 0x10030);
		p->fdi->rxtu[1] = snarfreg(igfx, o + 0x10038);

		p->fdi->chicken = snarfreg(igfx, o + 0x10064);

		p->fdi->dpll = &igfx->dpll[(igfx->dpllsel[0].v>>(x*4)) & 1];
		p->dpll = nil;
	} else {
		p->dpll = &igfx->dpll[x & 1];
	}

	/* display plane */
	p->dsp->cntr		= snarfreg(igfx, 0x70180 + x*0x1000);
	p->dsp->linoff		= snarfreg(igfx, 0x70184 + x*0x1000);
	p->dsp->stride		= snarfreg(igfx, 0x70188 + x*0x1000);
	p->dsp->tileoff		= snarfreg(igfx, 0x701A4 + x*0x1000);
	p->dsp->surf		= snarfreg(igfx, 0x7019C + x*0x1000);

	/* cursor plane */
	switch(igfx->type){
	case TypeIVB:
		p->cur->cntr	= snarfreg(igfx, 0x70080 + x*0x1000);
		p->cur->base	= snarfreg(igfx, 0x70084 + x*0x1000);
		p->cur->pos	= snarfreg(igfx, 0x70088 + x*0x1000);
		break;
	case TypeG45:
		p->dsp->pos	= snarfreg(igfx, 0x7018C + x*0x1000);
		p->dsp->size	= snarfreg(igfx, 0x70190 + x*0x1000);

		p->cur->cntr	= snarfreg(igfx, 0x70080 + x*0x40);
		p->cur->base	= snarfreg(igfx, 0x70084 + x*0x40);
		p->cur->pos	= snarfreg(igfx, 0x7008C + x*0x40);
		break;
	}
}

static int
devtype(Igfx *igfx)
{
	if(igfx->pci->vid != 0x8086)
		return -1;
	switch(igfx->pci->did){
	case 0x0a16:	/* HD 4400 - 4th Gen Core (ULT) */
		igfx->isult = 1;
		/* wet floor */
	case 0x0412:	/* HD 4600 - 4th Gen Core */
		return TypeHSW;
	case 0x0166:	/* 3rd Gen Core - ThinkPad X230 */
	case 0x0152:	/* 2nd/3rd Gen Core - Core-i3 */
		return TypeIVB;
	case 0x0046:	/* Thinkpad T510 */
	case 0x0102:	/* Dell Optiplex 790 */
	case 0x0126:	/* Thinkpad X220 */
		return TypeSNB;
	case 0x27a2:	/* GM945/82940GML - ThinkPad X60 Tablet */
	case 0x29a2:	/* 82P965/G965 HECI desktop */
	case 0x2a02:	/* GM965/GL960/X3100 - ThinkPad X61 Tablet */
	case 0x2a42:	/* 4 Series Mobile - ThinkPad X200 */
	case 0x2592:	/* 915GM */
		return TypeG45;
	}
	return -1;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	Igfx *igfx;
	int x, y;

	igfx = vga->private;
	if(igfx == nil) {
		igfx = alloc(sizeof(Igfx));
		igfx->ctlr = ctlr;
		igfx->pci = vga->pci;
		if(igfx->pci == nil){
			error("%s: no pci device\n", ctlr->name);
			return;
		}
		igfx->type = devtype(igfx);
		if(igfx->type < 0){
			error("%s: unrecognized device\n", ctlr->name);
			return;
		}
		vgactlpci(igfx->pci);
		if(1){
			vgactlw("type", ctlr->name);
			igfx->mmio = segattach(0, "igfxmmio", 0, igfx->pci->mem[0].size);
			if(igfx->mmio == (u32int*)-1)
				error("%s: segattach mmio failed: %r\n", ctlr->name);
		} else {
			if((igfx->pci->mem[4].bar & 1) == 0)
				error("%s: no pio bar\n", ctlr->name);
			igfx->pio = igfx->pci->mem[4].bar & ~1;
			igfx->mmio = nil;
		}
		vga->private = igfx;
	}

	switch(igfx->type){
	case TypeG45:
		igfx->npipe = 2;	/* A,B */

		igfx->dpll[0].ctrl	= snarfreg(igfx, 0x06014);
		igfx->dpll[0].fp0	= snarfreg(igfx, 0x06040);
		igfx->dpll[0].fp1	= snarfreg(igfx, 0x06044);
		igfx->dpll[1].ctrl	= snarfreg(igfx, 0x06018);
		igfx->dpll[1].fp0	= snarfreg(igfx, 0x06048);
		igfx->dpll[1].fp1	= snarfreg(igfx, 0x0604c);

		igfx->adpa		= snarfreg(igfx, 0x061100);
		igfx->lvds		= snarfreg(igfx, 0x061180);
		igfx->sdvob		= snarfreg(igfx, 0x061140);
		igfx->sdvoc		= snarfreg(igfx, 0x061160);

		igfx->pfit[0].ctrl	= snarfreg(igfx, 0x061230);
		y = (igfx->pfit[0].ctrl.v >> 29) & 3;
		if(igfx->pipe[y].pfit == nil)
			igfx->pipe[y].pfit = &igfx->pfit[0];

		igfx->vgacntrl		= snarfreg(igfx, 0x071400);
		break;

	case TypeSNB:
		igfx->npipe = 2;	/* A,B */
		igfx->cdclk = 300;	/* MHz */
		goto IVBcommon;

	case TypeIVB:
		igfx->npipe = 3;	/* A,B,C */
		igfx->cdclk = 400;	/* MHz */
		goto IVBcommon;

	IVBcommon:
		igfx->dpll[0].ctrl	= snarfreg(igfx, 0xC6014);
		igfx->dpll[0].fp0	= snarfreg(igfx, 0xC6040);
		igfx->dpll[0].fp1	= snarfreg(igfx, 0xC6044);
		igfx->dpll[1].ctrl	= snarfreg(igfx, 0xC6018);
		igfx->dpll[1].fp0	= snarfreg(igfx, 0xC6048);
		igfx->dpll[1].fp1	= snarfreg(igfx, 0xC604c);

		igfx->dpllsel[0]	= snarfreg(igfx, 0xC7000);

		igfx->drefctl		= snarfreg(igfx, 0xC6200);
		igfx->ssc4params	= snarfreg(igfx, 0xC6210);

		igfx->dp[0].ctl		= snarfreg(igfx, 0x64000);
		for(x=1; x<4; x++)
			igfx->dp[x].ctl	= snarfreg(igfx, 0xE4000 + 0x100*x);

		igfx->hdmi[1].ctl	= snarfreg(igfx, 0x0E1140);	/* HDMI_CTL_B */
		igfx->hdmi[1].bufctl[0]	= snarfreg(igfx, 0x0FC810);	/* HTMI_BUF_CTL_0 */
		igfx->hdmi[1].bufctl[1]	= snarfreg(igfx, 0x0FC81C);	/* HTMI_BUF_CTL_1 */
		igfx->hdmi[1].bufctl[2]	= snarfreg(igfx, 0x0FC828);	/* HTMI_BUF_CTL_2 */
		igfx->hdmi[1].bufctl[3]	= snarfreg(igfx, 0x0FC834);	/* HTMI_BUF_CTL_3 */

		igfx->hdmi[2].ctl	= snarfreg(igfx, 0x0E1150);	/* HDMI_CTL_C */
		igfx->hdmi[2].bufctl[0]	= snarfreg(igfx, 0x0FCC00);	/* HTMI_BUF_CTL_4 */
		igfx->hdmi[2].bufctl[1]	= snarfreg(igfx, 0x0FCC0C);	/* HTMI_BUF_CTL_5 */
		igfx->hdmi[2].bufctl[2]	= snarfreg(igfx, 0x0FCC18);	/* HTMI_BUF_CTL_6 */
		igfx->hdmi[2].bufctl[3]	= snarfreg(igfx, 0x0FCC24);	/* HTMI_BUF_CTL_7 */

		igfx->hdmi[3].ctl	= snarfreg(igfx, 0x0E1160);	/* HDMI_CTL_D */
		igfx->hdmi[3].bufctl[0]	= snarfreg(igfx, 0x0FD000);	/* HTMI_BUF_CTL_8 */
		igfx->hdmi[3].bufctl[1]	= snarfreg(igfx, 0x0FD00C);	/* HTMI_BUF_CTL_9 */
		igfx->hdmi[3].bufctl[2]	= snarfreg(igfx, 0x0FD018);	/* HTMI_BUF_CTL_10 */
		igfx->hdmi[3].bufctl[3]	= snarfreg(igfx, 0x0FD024);	/* HTMI_BUF_CTL_11 */

		igfx->lvds		= snarfreg(igfx, 0x0E1180);	/* LVDS_CTL */
		goto PCHcommon;

	case TypeHSW:
		igfx->npipe = 4;	/* A,B,C,eDP */
		igfx->cdclk = igfx->isult ? 450 : 540;	/* MHz */

		igfx->dpll[0].ctrl	= snarfreg(igfx, 0x130040);	/* LCPLL_CTL */
		igfx->dpll[1].ctrl	= snarfreg(igfx, 0x46040);	/* WRPLL_CTL1 */
		igfx->dpll[2].ctrl	= snarfreg(igfx, 0x46060);	/* WRPLL_CTL2 */
		igfx->dpll[3].ctrl	= snarfreg(igfx, 0x46020);	/* SPLL_CTL */

		for(x=0; x<nelem(igfx->dp); x++){
			if(x == 3 && igfx->isult)	/* no DDI D */
				continue;
			igfx->dp[x].bufctl 	= snarfreg(igfx, 0x64000 + 0x100*x);
			igfx->dp[x].ctl 	= snarfreg(igfx, 0x64040 + 0x100*x);
			if(x > 0)
				igfx->dp[x].stat	= snarfreg(igfx, 0x64044 + 0x100*x);
			for(y=0; y<nelem(igfx->dp[x].buftrans); y++)
				igfx->dp[x].buftrans[y]	= snarfreg(igfx, 0x64E00 + 0x60*x + 4*y);
			igfx->dpllsel[x]	= snarfreg(igfx, 0x46100 + 4*x);
		}

		goto PCHcommon;

	PCHcommon:
		igfx->rawclkfreq	= snarfreg(igfx, 0xC6204);

		/* cpu displayport A */
		igfx->dp[0].auxctl	= snarfreg(igfx, 0x64010);
		igfx->dp[0].auxdat[0]	= snarfreg(igfx, 0x64014);
		igfx->dp[0].auxdat[1]	= snarfreg(igfx, 0x64018);
		igfx->dp[0].auxdat[2]	= snarfreg(igfx, 0x6401C);
		igfx->dp[0].auxdat[3]	= snarfreg(igfx, 0x64020);
		igfx->dp[0].auxdat[4]	= snarfreg(igfx, 0x64024);

		/* pch displayport B,C,D */
		for(x=1; x<4; x++){
			igfx->dp[x].auxctl	= snarfreg(igfx, 0xE4010 + 0x100*x);
			igfx->dp[x].auxdat[0]	= snarfreg(igfx, 0xE4014 + 0x100*x);
			igfx->dp[x].auxdat[1]	= snarfreg(igfx, 0xE4018 + 0x100*x);
			igfx->dp[x].auxdat[2]	= snarfreg(igfx, 0xE401C + 0x100*x);
			igfx->dp[x].auxdat[3]	= snarfreg(igfx, 0xE4020 + 0x100*x);
			igfx->dp[x].auxdat[4]	= snarfreg(igfx, 0xE4024 + 0x100*x);
		}

		for(x=0; x<igfx->npipe; x++){
			if(igfx->type == TypeHSW && x == 3){
				igfx->pipe[x].pfit = &igfx->pfit[0];
				continue;
			}
			igfx->pfit[x].pwrgate	= snarfreg(igfx, 0x68060 + 0x800*x);
			igfx->pfit[x].winpos	= snarfreg(igfx, 0x68070 + 0x800*x);
			igfx->pfit[x].winsize	= snarfreg(igfx, 0x68074 + 0x800*x);
			igfx->pfit[x].ctrl	= snarfreg(igfx, 0x68080 + 0x800*x);

			y = (igfx->pfit[x].ctrl.v >> 29) & 3;
			if(igfx->pipe[y].pfit == nil)
				igfx->pipe[y].pfit = &igfx->pfit[x];
		}

		igfx->ppstatus		= snarfreg(igfx, 0xC7200);
		igfx->ppcontrol		= snarfreg(igfx, 0xC7204);

		for(x=0; x<5; x++)
			igfx->gmbus[x]	= snarfreg(igfx, 0xC5100 + x*4);
		igfx->gmbus[x]	= snarfreg(igfx, 0xC5120);

		igfx->adpa		= snarfreg(igfx, 0x0E1100);	/* DAC_CTL */
		igfx->vgacntrl		= snarfreg(igfx, 0x041000);
		break;
	}

	for(x=0; x<igfx->npipe; x++)
		snarfpipe(igfx, x);

	for(x=0; x<nelem(vga->edid); x++){
		Modelist *l;

		switch(x){
		case PortVGA:
			vga->edid[x] = snarfgmedid(igfx, 2, 0x50);
			break;
		case PortLCD:
			vga->edid[x] = snarfgmedid(igfx, 3, 0x50);
			if(vga->edid[x] == nil)
				continue;
			for(l = vga->edid[x]->modelist; l != nil; l = l->next)
				l->attr = mkattr(l->attr, "lcd", "1");
			break;
		case PortDPD:
			if(igfx->type == TypeHSW && igfx->isult)
				continue;
		case PortDPA:
		case PortDPB:
		case PortDPC:
			vga->edid[x] = snarfdpedid(igfx, &igfx->dp[x-PortDPA], 0x50);
			break;
		default:
			continue;
		}

		if(vga->edid[x] == nil){
			if(x < PortDPB)
				continue;
			vga->edid[x] = snarfgmedid(igfx, x + 1 & ~1 | x >> 1 & 1, 0x50);
			if(vga->edid[x] == nil)
				continue;
			igfx->dp[x-PortDPA].hdmi = 1;
		}
		for(l = vga->edid[x]->modelist; l != nil; l = l->next)
			l->attr = mkattr(l->attr, "display", "%d", x+1);
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	USED(vga);
	ctlr->flag |= Hlinear|Ulinear|Foptions;
}

enum {
	Lcpll = 2700,
	Lcpll2k = Lcpll * 2000,
};

static int
wrbudget(int freq)
{
	static int b[] = {
		25175000,0, 25200000,0, 27000000,0, 27027000,0,
		37762500,0, 37800000,0, 40500000,0, 40541000,0,
		54000000,0, 54054000,0, 59341000,0, 59400000,0,
		72000000,0, 74176000,0, 74250000,0, 81000000,0,
		81081000,0, 89012000,0, 89100000,0, 108000000,0,
		108108000,0, 111264000,0, 111375000,0, 148352000,0,
		148500000,0, 162000000,0, 162162000,0, 222525000,0,
		222750000,0, 296703000,0, 297000000,0, 233500000,1500,
		245250000,1500, 247750000,1500, 253250000,1500, 298000000,1500,
		169128000,2000, 169500000,2000, 179500000,2000, 202000000,2000,
		256250000,4000, 262500000,4000, 270000000,4000, 272500000,4000,
		273750000,4000, 280750000,4000, 281250000,4000, 286000000,4000,
		291750000,4000, 267250000,5000, 268500000,5000
	};
	int *i;

	for(i=b; i<b+nelem(b); i+=2)
		if(i[0] == freq)
			return i[1];
	return 1000;
}

static void
genwrpll(int freq, int *n2, int *p, int *r2)
{
	int budget, N2, P, R2;
	vlong f2k, a, b, c, d, Δ, bestΔ;

	f2k = freq / 100;
	if(f2k == Lcpll2k){	/* bypass wrpll entirely and use lcpll */
		*n2 = 2;
		*p = 1;
		*r2 = 2;
		return;
	}
	budget = wrbudget(freq);
	*p = 0;
	for(R2=Lcpll*2/400+1; R2<=Lcpll*2/48; R2++)
	for(N2=2400*R2/Lcpll+1; N2<=4800*R2/Lcpll; N2++)
		for(P=2; P<=64; P+=2){
			if(*p == 0){
				*n2 = N2;
				*p = P;
				*r2 = R2;
				continue;
			}
			Δ = f2k * P * R2;
			Δ -= N2 * Lcpll2k;
			if(Δ < 0)
				Δ = -Δ;
			bestΔ = f2k * *p * *r2;
			bestΔ -= *n2 * Lcpll2k;
			if(bestΔ < 0)
				bestΔ = -bestΔ;
			a = f2k * budget;
			b = a;
			a *= P * R2;
			b *= *p * *r2;
			c = Δ * MHz;
			d = bestΔ * MHz;
			if(a < c && b < d && *p * *r2 * Δ < P * R2 * bestΔ
			|| a >= c && b < d
			|| a >= c && b >= d && N2 * *r2 * *r2 > *n2 * R2 * R2){
				*n2 = N2;
				*p = P;
				*r2 = R2;
			}
		}
}

static int
genpll(int freq, int cref, int P2, int *m1, int *m2, int *n, int *p1)
{
	int M1, M2, M, N, P, P1;
	int best, error;
	vlong a;

	best = -1;
	for(N=3; N<=8; N++)
	for(M2=5; M2<=9; M2++)
	for(M1=10; M1<=20; M1++){
		M = 5*(M1+2) + (M2+2);
		if(M < 70 || M > 120)
			continue;
		for(P1=1; P1<=8; P1++){
			P = P1 * P2;
			if(P < 4 || P > 98)
				continue;
			a = cref;
			a *= M;
			a /= N+2;
			a /= P;
			if(a < 20*MHz || a > 400*MHz)
				continue;
			error = a;
			error -= freq;
			if(error < 0)
				error = -error;
			if(best < 0 || error < best){
				best = error;
				*m1 = M1;
				*m2 = M2;
				*n = N;
				*p1 = P1;
			}
		}
	}
	return best;
}

static int
initdpll(Igfx *igfx, int x, int freq, int islvds, int ishdmi)
{
	int cref, m1, m2, n, n2, p1, p2, r2;
	Dpll *dpll;

	switch(igfx->type){
	case TypeG45:
		/* PLL Reference Input Select */
		dpll = igfx->pipe[x].dpll;
		dpll->ctrl.v &= ~(3<<13);
		dpll->ctrl.v |= (islvds ? 2 : 0) << 13;
		cref = islvds ? 100*MHz : 96*MHz;
		break;
	case TypeIVB:
		/* transcoder dpll enable */
		igfx->dpllsel[0].v |= 8<<(x*4);
		/* program rawclock to 125MHz */
		igfx->rawclkfreq.v = 125;
		if(islvds){
			/* 120MHz SSC integrated source enable */
			igfx->drefctl.v &= ~(3<<11);
			igfx->drefctl.v |= 2<<11;

			/* 120MHz SSC4 modulation en */	
			igfx->drefctl.v |= 2;
		}
		/*
		 * PLL Reference Input Select:
		 * 000	DREFCLK		(default is 120 MHz) for DAC/HDMI/DVI/DP
		 * 001	Super SSC	120MHz super-spread clock
		 * 011	SSC		Spread spectrum input clock (120MHz default) for LVDS/DP
		 */
		dpll = igfx->pipe[x].fdi->dpll;
		dpll->ctrl.v &= ~(7<<13);
		dpll->ctrl.v |= (islvds ? 3 : 0) << 13;
		cref = 120*MHz;
		break;
	case TypeHSW:
		/* select port clock to pipe */
		igfx->pipe[x].clksel.v = port+1-PortDPA<<29;

		if(igfx->dp[port-PortDPA].hdmi){
			/* select port clock */
			igfx->dpllsel[port-PortDPA].v = 1<<31;	/* WRPLL1 */
			igfx->pipe[x].dpll = &igfx->dpll[1];

			dpll = igfx->pipe[x].dpll;
			/* enable pll */
			dpll->ctrl.v = 1<<31;
			/* LCPLL 2700 (non scc) reference */
			dpll->ctrl.v |= 3<<28;

			genwrpll(freq, &n2, &p1, &r2);
			dpll->ctrl.v |= n2 << 16;
			dpll->ctrl.v |= p1 << 8;
			dpll->ctrl.v |= r2;
		}else{
			/* select port clock */
			igfx->dpllsel[port-PortDPA].v = 1<<29;	/* LCPLL 1350 */
			/* keep preconfigured frequency settings, keep cdclk */
			dpll = igfx->pipe[x].dpll;
			dpll->ctrl.v &= ~(1<<31) & ~(1<<28) & ~(1<<26);
		}
		return 0;
	default:
		return -1;
	}

	/* Dpll Mode select */
	dpll->ctrl.v &= ~(3<<26);
	dpll->ctrl.v |= (islvds ? 2 : 1)<<26;

	/* P2 Clock Divide */
	dpll->ctrl.v &= ~(3<<24);	
	if(islvds){
		p2 = 14;
		if(genpll(freq, cref, p2, &m1, &m2, &n, &p1) < 0)
			return -1;
	} else {
		p2 = 10;
		if(freq > 270*MHz){
			p2 >>= 1;
			dpll->ctrl.v |= (1<<24);
		}
		if(genpll(freq, cref, p2, &m1, &m2, &n, &p1) < 0)
			return -1;
	}

	/* Dpll VCO Enable */
	dpll->ctrl.v |= (1<<31);

	/* Dpll Serial DVO High Speed IO clock Enable */
	if(ishdmi)
		dpll->ctrl.v |= (1<<30);
	else
		dpll->ctrl.v &= ~(1<<30);

	/* VGA Mode Disable */
	dpll->ctrl.v |= (1<<28);

	/* P1 Post Divisor */
	dpll->ctrl.v &= ~0xFF00FF;
	dpll->ctrl.v |= 0x10001<<(p1-1);

	dpll->fp0.v &= ~(0x3f<<16);
	dpll->fp0.v |= n << 16;
	dpll->fp0.v &= ~(0x3f<<8);
	dpll->fp0.v |= m1 << 8;
	dpll->fp0.v &= ~(0x3f<<0);
	dpll->fp0.v |= m2 << 0;

	/* FP0 P1 Post Divisor */
	dpll->ctrl.v &= ~0xFF0000;
	dpll->ctrl.v |=  0x010000<<(p1-1);

	/* FP1 P1 Post divisor */
	if(igfx->pci->did != 0x27a2 && igfx->pci->did != 0x2592){
		dpll->ctrl.v &= ~0xFF;
		dpll->ctrl.v |=  0x01<<(p1-1);
		dpll->fp1.v = dpll->fp0.v;
	}

	return 0;
}

static void
initdatalinkmn(Trans *t, int freq, int lsclk, int lanes, int tu, int bpp)
{
	uvlong m, n;

	n = 0x800000;
	m = (n * ((freq * bpp)/8)) / (lsclk * lanes);
	t->dm[0].v = (tu-1)<<25 | m;
	t->dn[0].v = n;

	n = 0x80000;
	m = (n * freq) / lsclk;
	t->lm[0].v = m;
	t->ln[0].v = n;

	t->dm[1].v = t->dm[0].v;
	t->dn[1].v = t->dn[0].v;
	t->lm[1].v = t->lm[0].v;
	t->ln[1].v = t->ln[0].v;
}

static void
inittrans(Trans *t, Mode *m)
{
	/* tans/pipe enable */
	t->conf.v = 1<<31;

	/* trans/pipe timing */
	t->ht.v = (m->ht - 1)<<16 | (m->x - 1);
	t->hb.v = t->ht.v;
	t->hs.v = (m->ehs - 1)<<16 | (m->shs - 1);
	t->vt.v = (m->vt - 1)<<16 | (m->y - 1);
	t->vb.v = t->vt.v;
	t->vs.v = (m->vre - 1)<<16 | (m->vrs - 1);
	t->vss.v = 0;
}

static void
initpipe(Igfx *igfx, Pipe *p, Mode *m, int bpc, int port)
{
	static uchar bpctab[4] = { 8, 10, 6, 12 };
	int i, tu, bpc, lanes;
	Fdi *fdi;

	/* source image size */
	p->src.v = (m->x - 1)<<16 | (m->y - 1);

	if(p->pfit != nil){
		/* panel fitter enable, hardcoded coefficients */
		p->pfit->ctrl.v = 1<<31 | 1<<23;
		p->pfit->winpos.v = 0;
		p->pfit->winsize.v = (m->x << 16) | m->y;
	}

	/* enable and set monitor timings for cpu pipe */
	inittrans(p, m);

	/* default for displayport */
	tu = 64;
	bpc = 8;
	lanes = 1;

	fdi = p->fdi;
	if(fdi->rxctl.a != 0){
		/* enable and set monitor timings for transcoder */
		inittrans(fdi, m);

		/*
		 * hack:
		 * we do not program fdi in load(), so use
		 * snarfed bios initialized values for now.
		 */
		if(fdi->rxctl.v & (1<<31)){
			tu = 1+(fdi->rxtu[0].v >> 25);
			bpc = bpctab[(fdi->rxctl.v >> 16) & 3];
			lanes = 1+((fdi->rxctl.v >> 19) & 7);
		}

		/* fdi tx enable */
		fdi->txctl.v |= (1<<31);
		/* tx port width selection */
		fdi->txctl.v &= ~(7<<19);
		fdi->txctl.v |= (lanes-1)<<19;
		/* tx fdi pll enable */
		fdi->txctl.v |= (1<<14);
		/* clear auto training bits */
		fdi->txctl.v &= ~(7<<8 | 1);

		/* fdi rx enable */
		fdi->rxctl.v |= (1<<31);
		/* rx port width selection */
		fdi->rxctl.v &= ~(7<<19);
		fdi->rxctl.v |= (lanes-1)<<19;
		/* bits per color for transcoder */
		for(i=0; i<nelem(bpctab); i++){
			if(bpctab[i] == bpc){
				fdi->rxctl.v &= ~(7<<16);
				fdi->rxctl.v |= i<<16;
				fdi->dpctl.v &= ~(7<<9);
				fdi->dpctl.v |= i<<9;
				break;
			}
		}
		/* rx fdi pll enable */
		fdi->rxctl.v |= (1<<13);
		/* rx fdi rawclk to pcdclk selection */
		fdi->rxctl.v |= (1<<4);

		/* tusize 1 and 2 */
		fdi->rxtu[0].v = (tu-1)<<25;
		fdi->rxtu[1].v = (tu-1)<<25;
		initdatalinkmn(fdi, m->frequency, 270*MHz, lanes, tu, 3*bpc);
	}else if(igfx->type == TypeHSW){
		p->dpctl.v &= 0xf773733e;	/* mbz */
		/* transcoder enable */
		p->dpctl.v |= 1<<31;
		/* DDI select (ignored by eDP) */
		p->dpctl.v &= ~(7<<28);
		p->dpctl.v |= (port-PortDPA)<<28;
		/* displayport SST or hdmi mode */
		p->dpctl.v &= ~(7<<24);
		if(!igfx->dp[port-PortDPA].hdmi)
			p->dpctl.v |= 2<<24;
		/* sync polarity */
		p->dpctl.v |= 3<<16;
		if(m->hsync == '-')
			p->dpctl.v ^= 1<<16;
		if(m->vsync == '-')
			p->dpctl.v ^= 1<<17;
		/* eDP input select: always on power well */
		p->dpctl.v &= ~(7<<12);
		/* dp port width */
		if(igfx->dp[port-PortDPA].hdmi)
			lanes = 4;
		else{
			p->dpctl.v &= ~(7<<1);
			p->dpctl.v |= lanes-1 << 1;
		}
	}

	/* bits per color for cpu pipe */
	for(i=0; i<nelem(bpctab); i++){
		if(bpctab[i] == bpc){
			if(igfx->type == TypeHSW){
				p->dpctl.v &= ~(7<<20);
				p->dpctl.v |= i<<20;
			}else{
				p->conf.v &= ~(7<<5);
				p->conf.v |= i<<5;
			}
			break;
		}
	}
	initdatalinkmn(p, m->frequency, 270*MHz, lanes, tu, 3*bpc);
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	int x, nl, port, bpc;
	char *val;
	Igfx *igfx;
	Pipe *p;
	Mode *m;

	m = vga->mode;
	if(m->z != 32)
		error("%s: unsupported color depth %d\n", ctlr->name, m->z);

	igfx = vga->private;

	/* disable vga */
	igfx->vgacntrl.v |= (1<<31);

	/* disable all pipes and ports */
	igfx->ppcontrol.v &= 10;	/* 15:4 mbz; unset 5<<0 */
	igfx->lvds.v &= ~(1<<31);
	igfx->adpa.v &= ~(1<<31);
	if(igfx->type == TypeG45)
		igfx->adpa.v |= (3<<10);	/* Monitor DPMS: off */
	if(igfx->type == TypeHSW){
		for(x=1; x<nelem(igfx->dpll); x++)
			igfx->dpll[x].ctrl.v &= ~(1<<31);
		for(x=0; x<nelem(igfx->dpllsel); x++)
			igfx->dpllsel[x].v = 7<<29;
	}
	for(x=0; x<nelem(igfx->dp); x++){
		igfx->dp[x].ctl.v &= ~(1<<31);
		igfx->dp[x].bufctl.v &= ~(1<<31);
	}
	for(x=0; x<nelem(igfx->hdmi); x++)
		igfx->hdmi[x].ctl.v &= ~(1<<31);
	for(x=0; x<igfx->npipe; x++){
		/* disable displayport transcoders */
		igfx->pipe[x].dpctl.v &= ~(1<<31);
		igfx->pipe[x].fdi->dpctl.v &= ~(1<<31);
		if(igfx->type == TypeHSW){
			igfx->pipe[x].dpctl.v &= ~(7<<28);
			igfx->pipe[x].fdi->dpctl.v &= ~(7<<28);
		}else{
			igfx->pipe[x].dpctl.v |= (3<<29);
			igfx->pipe[x].fdi->dpctl.v |= (3<<29);
		}
		/* disable transcoder/pipe */
		igfx->pipe[x].conf.v &= ~(1<<31);
		igfx->pipe[x].fdi->conf.v &= ~(1<<31);
	}

	if((val = dbattr(m->attr, "display")) != nil){
		port = atoi(val)-1;
		if(igfx->type == TypeHSW && !igfx->dp[port-PortDPA].hdmi)
			bpc = 6;
	}else if(dbattr(m->attr, "lcd") != nil)
		port = PortLCD;
	else
		port = PortVGA;

	trace("%s: display #%d\n", ctlr->name, port+1);

	switch(port){
	default:
	Badport:
		error("%s: display #%d not supported\n", ctlr->name, port+1);
		break;

	case PortVGA:
		if(igfx->type == TypeHSW)	/* unimplemented */
			goto Badport;
		if(igfx->type == TypeG45)
			x = (igfx->adpa.v >> 30) & 1;
		else
			x = (igfx->adpa.v >> 29) & 3;
		igfx->adpa.v |= (1<<31);
		if(igfx->type == TypeG45){
			igfx->adpa.v &= ~(3<<10);	/* Monitor DPMS: on */

			igfx->adpa.v &= ~(1<<15);	/* ADPA Polarity Select */
			igfx->adpa.v |= 3<<3;
			if(m->hsync == '-')
				igfx->adpa.v ^= 1<<3;
			if(m->vsync == '-')
				igfx->adpa.v ^= 1<<4;
		}
		break;

	case PortLCD:
		if(igfx->type == TypeHSW)
			goto Badport;
		if(igfx->type == TypeG45)
			x = (igfx->lvds.v >> 30) & 1;
		igfx->lvds.v |= (1<<31);

			igfx->lvds.v |= (1<<15);	/* border enable */
		}
		break;

	case PortDPA:
	case PortDPB:
	case PortDPC:
	case PortDPD:
	case PortDPE:
		r = &igfx->dp[port - PortDPA].ctl;
		if(r->a == 0)
			goto Badport;
		/* port enable */
		r->v |= 1<<31;
		/* use PIPE_A for displayport */
		x = 0;

		if(igfx->type == TypeHSW){
			if(port == PortDPA){
				/* only enable panel for eDP */
				igfx->ppcontrol.v |= 5;
				/* use eDP pipe */
				x = 3;
			}

			/* reserved MBZ */
			r->v &= ~(7<<28) & ~(1<<26) & ~(63<<19) & ~(3<<16) & ~(15<<11) & ~(1<<7) & ~31;
			/* displayport SST mode */
			r->v &= ~(1<<27);
			/* link not in training, send normal pixels */
			r->v |= 3<<8;
			if(igfx->dp[port-PortDPA].hdmi){
				/* hdmi: do not configure displayport */
				r->a = 0;
				r->v &= ~(1<<31);
			}

			r = &igfx->dp[port - PortDPA].bufctl;
			/* buffer enable */
			r->v |= 1<<31;
			/* reserved MBZ */
			r->v &= ~(7<<28) & ~(127<<17) & ~(255<<8) & ~(3<<5);
			/* grab lanes shared with port e when using port a */
			if(port == PortDPA)
				r->v |= 1<<4;
			else if(port == PortDPE)
				igfx->dp[0].bufctl.v &= ~(1<<4);
			/* dp port width */
			r->v &= ~(15<<1);
			if(!igfx->dp[port-PortDPA].hdmi){
				nl = needlanes(m->frequency, 270*MHz, 3*bpc);
				/* x4 unsupported on port e */
				if(nl > 2 && port == PortDPE)
					goto Badport;
				r->v |= nl-1 << 1;
			}
			/* port reversal: off */
			r->v &= ~(1<<16);

			/* buffer translation (vsl/pel) */
			r = igfx->dp[port - PortDPA].buftrans;
			r[1].v  = 0x0006000E;	r[0].v  = 0x00FFFFFF;
			r[3].v  = 0x0005000A;	r[2].v  = 0x00D75FFF;
			r[5].v  = 0x00040006;	r[4].v  = 0x00C30FFF;
			r[7].v  = 0x000B0000;	r[6].v  = 0x80AAAFFF;
			r[9].v  = 0x0005000A;	r[8].v  = 0x00FFFFFF;
			r[11].v = 0x000C0004;	r[10].v = 0x00D75FFF;
			r[13].v = 0x000B0000;	r[12].v = 0x80C30FFF;
			r[15].v = 0x00040006;	r[14].v = 0x00FFFFFF;
			r[17].v = 0x000B0000;	r[16].v = 0x80D75FFF;
			r[19].v = 0x00040006;	r[18].v = 0x00FFFFFF;
			break;
		}

		if(port == PortDPE)
			goto Badport;

		/* port width selection */
		r->v &= ~(7<<19);
		r->v |= needlanes(m->frequency, 270*MHz, 3*bpc)-1 << 19;

		/* port reversal: off */
		r->v &= ~(1<<15);
		/* reserved MBZ */
		r->v &= ~(15<<11);
		/* displayport transcoder */
		if(port == PortDPA){
			/* reserved MBZ */
			r->v &= ~(15<<10);
			/* pll frequency: 270mhz */
			r->v &= ~(3<<16);
			/* pll enable */
			r->v |= 1<<14;
			/* pipe select */
			r->v &= ~(3<<29);
			r->v |= x<<29;
		} else if(igfx->pipe[x].fdi->dpctl.a != 0){
			/* reserved MBZ */
			r->v &= ~(15<<11);
			/* audio output: disable */
			r->v &= ~(1<<6);
			/* transcoder displayport configuration */
			r = &igfx->pipe[x].fdi->dpctl;
			/* transcoder enable */
			r->v |= 1<<31;
			/* port select: B,C,D */
			r->v &= ~(3<<29);
			r->v |= (port-PortDPB)<<29;
		}
		/* sync polarity */
		r->v |= 3<<3;
		if(m->hsync == '-')
			r->v ^= 1<<3;
		if(m->vsync == '-')
			r->v ^= 1<<4;
		break;
	}
	p = &igfx->pipe[x];

	/* plane enable, 32bpp */
	p->dsp->cntr.v = (1<<31) | (6<<26);
	if(igfx->type == TypeG45)
		p->dsp->cntr.v |= x<<24;	/* pipe assign */
	else
		p->dsp->cntr.v &= ~511;	/* mbz */

	/* stride must be 64 byte aligned */
	p->dsp->stride.v = m->x * (m->z / 8);
	p->dsp->stride.v += 63;
	p->dsp->stride.v &= ~63;

	/* virtual width in pixels */
	vga->virtx = p->dsp->stride.v / (m->z / 8);

	p->dsp->surf.v = 0;
	p->dsp->linoff.v = 0;
	p->dsp->tileoff.v = 0;
	p->dsp->leftsurf.v = 0;

	/* cursor plane off */
	p->cur->cntr.v = x<<28;
	p->cur->pos.v = 0;
	p->cur->base.v = 0;

	if(initdpll(igfx, x, m->frequency, islvds, 0) < 0)
		error("%s: frequency %d out of range\n", ctlr->name, m->frequency);

	initpipe(igfx, p, m, bpc, port);

	ctlr->flag |= Finit;
}

static void
loadtrans(Igfx *igfx, Trans *t)
{
	int i;

	if(t->conf.a == 0)
		return;

	/* program trans/pipe timings */
	loadreg(igfx, t->ht);
	loadreg(igfx, t->hb);
	loadreg(igfx, t->hs);
	loadreg(igfx, t->vt);
	loadreg(igfx, t->vb);
	loadreg(igfx, t->vs);
	loadreg(igfx, t->vss);

	loadreg(igfx, t->dm[0]);
	loadreg(igfx, t->dn[0]);
	loadreg(igfx, t->lm[0]);
	loadreg(igfx, t->ln[0]);
	loadreg(igfx, t->dm[1]);
	loadreg(igfx, t->dn[1]);
	loadreg(igfx, t->lm[1]);
	loadreg(igfx, t->ln[1]);

	if(t->dpll != nil && igfx->type != TypeHSW){
		/* program dpll */
		t->dpll->ctrl.v &= ~(1<<31);
		loadreg(igfx, t->dpll->ctrl);
		loadreg(igfx, t->dpll->fp0);
		loadreg(igfx, t->dpll->fp1);

		/* enable dpll */
		t->dpll->ctrl.v |= (1<<31);
		loadreg(igfx, t->dpll->ctrl);
		sleep(10);
	}

	/* workarround: set timing override bit */
	csr(igfx, t->chicken.a, 0, 1<<31);

	/* enable trans/pipe */
	t->conf.v |= (1<<31);
	t->conf.v &= ~(1<<30);
	loadreg(igfx, t->conf);
	for(i=0; i<100; i++){
		sleep(10);
		if(rr(igfx, t->conf.a) & (1<<30))
			break;
	}
}

static void
enablepipe(Igfx *igfx, int x)
{
	Pipe *p;

	p = &igfx->pipe[x];
	if((p->conf.v & (1<<31)) == 0)
		return;	/* pipe is disabled, done */

	/* select pipe clock */
	loadreg(igfx, p->clksel);

	if(p->fdi->rxctl.a != 0){
		p->fdi->rxctl.v &= ~(1<<31);
		loadreg(igfx, p->fdi->rxctl);
		sleep(5);
		p->fdi->txctl.v &= ~(1<<31);
		loadreg(igfx, p->fdi->txctl);
		sleep(5);
	}

	/* image size (vga needs to be off) */
	loadreg(igfx, p->src);

	/* set panel fitter as needed */
	if(p->pfit != nil){
		loadreg(igfx, p->pfit->ctrl);
		loadreg(igfx, p->pfit->winpos);
		loadreg(igfx, p->pfit->winsize);	/* arm */
	}

	/* enable cpu pipe */
	loadtrans(igfx, p);

	/* program plane */
	loadreg(igfx, p->dsp->cntr);
	loadreg(igfx, p->dsp->linoff);
	loadreg(igfx, p->dsp->stride);
	loadreg(igfx, p->dsp->tileoff);
	loadreg(igfx, p->dsp->surf);	/* arm */
	loadreg(igfx, p->dsp->leftsurf);

	/* program cursor */
	loadreg(igfx, p->cur->cntr);
	loadreg(igfx, p->cur->pos);
	loadreg(igfx, p->cur->base);	/* arm */

	if(0){
		/* enable fdi */
		loadreg(igfx, p->fdi->rxtu[1]);
		loadreg(igfx, p->fdi->rxtu[0]);
		loadreg(igfx, p->fdi->rxmisc);
		p->fdi->rxctl.v &= ~(3<<8 | 1<<10 | 3);
		p->fdi->rxctl.v |= (1<<31);
		loadreg(igfx, p->fdi->rxctl);

		p->fdi->txctl.v &= ~(3<<8 | 1<<10 | 2);
		p->fdi->txctl.v |= (1<<31);
		loadreg(igfx, p->fdi->txctl);
	}

	/* enable the transcoder */
	loadtrans(igfx, p->fdi);
}

static void
disabletrans(Igfx *igfx, Trans *t)
{
	int i;

	/* deselect pipe clock */
	csr(igfx, t->clksel.a, 7<<29, 0);

	/* disable displayport transcoder */
	if(igfx->type == TypeHSW){
		csr(igfx, t->dpctl.a, 15<<28, 0);
		csr(igfx, t->ht.a, ~0, 0);
		csr(igfx, t->hb.a, ~0, 0);
		csr(igfx, t->hs.a, ~0, 0);
		csr(igfx, t->vt.a, ~0, 0);
		csr(igfx, t->vb.a, ~0, 0);
		csr(igfx, t->vs.a, ~0, 0);
		csr(igfx, t->vss.a, ~0, 0);
	}else
		csr(igfx, t->dpctl.a, 1<<31, 3<<29);

	/* disable transcoder / pipe */
	csr(igfx, t->conf.a, 1<<31, 0);
	for(i=0; i<100; i++){
		sleep(10);
		if((rr(igfx, t->conf.a) & (1<<30)) == 0)
			break;
	}
	/* workarround: clear timing override bit */
	csr(igfx, t->chicken.a, 1<<31, 0);

skippipe:
	/* disable dpll  */
	if(igfx->type != TypeHSW && t->dpll != nil)
		csr(igfx, t->dpll->ctrl.a, 1<<31, 0);
}

static void
disablepipe(Igfx *igfx, int x)
{
	Pipe *p;

	p = &igfx->pipe[x];

	/* planes off */
	csr(igfx, p->dsp->cntr.a, 1<<31, 0);
	csr(igfx, p->dsp->surf.a, ~0, 0);	/* arm */
	/* cursor off */
	if(igfx->type == TypeHSW)
		csr(igfx, p->cur->cntr.a, 0x3F, 0);
	else
		csr(igfx, p->cur->cntr.a, 1<<5 | 7, 0);
	wr(igfx, p->cur->base.a, 0);	/* arm */

	/* display/overlay/cursor planes off */
	if(igfx->type == TypeG45)
		csr(igfx, p->conf.a, 0, 3<<18);
	csr(igfx, p->src.a, ~0, 0);

	/* disable cpu pipe */
	disabletrans(igfx, p);

	/* disable panel fitter */
	if(p->pfit != nil)
		csr(igfx, p->pfit->ctrl.a, 1<<31, 0);

	if(0){
		/* disable fdi transmitter and receiver */
		csr(igfx, p->fdi->txctl.a, 1<<31 | 1<<10, 0);
		csr(igfx, p->fdi->rxctl.a, 1<<31 | 1<<10, 0);
	}

	/* disable displayport transcoder */
	csr(igfx, p->fdi->dpctl.a, 1<<31, 3<<29);

	/* disable pch transcoder */
	disabletrans(igfx, p->fdi);

	/* disable pch dpll enable bit */
	if(igfx->type != TypeHSW)
		csr(igfx, igfx->dpllsel[0].a, 8<<(x*4), 0);
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	Igfx *igfx;
	int x, y;

	igfx = vga->private;

	/* power lcd off */
	if(igfx->ppcontrol.a != 0){
		csr(igfx, igfx->ppcontrol.a, 0xFFFF0005, 0xABCD0000);
		for(x=0; x<5000; x++){
			sleep(10);
			if((rr(igfx, igfx->ppstatus.a) & (1<<31)) == 0)
				break;
		}
	}

	/* disable ports */
	csr(igfx, igfx->sdvob.a, (1<<29) | (1<<31), 0);
	csr(igfx, igfx->sdvoc.a, (1<<29) | (1<<31), 0);
	csr(igfx, igfx->adpa.a, 1<<31, 0);
	csr(igfx, igfx->lvds.a, 1<<31, 0);
	for(x = 0; x < nelem(igfx->dp); x++){
		csr(igfx, igfx->dp[x].ctl.a, 1<<31, 0);
		if(igfx->dp[x].bufctl.a != 0){
			csr(igfx, igfx->dp[x].bufctl.a, 1<<31, 0);
			/* wait for buffers to return to idle */
			for(y=0; y<5; y++){
				sleep(10);
				if(rr(igfx, igfx->dp[x].bufctl.a) & 1<<7)
					break;
			}
		}
	}
	for(x = 0; x < nelem(igfx->hdmi); x++)
		csr(igfx, igfx->hdmi[x].ctl.a, 1<<31, 0);

	/* disable vga plane */
	csr(igfx, igfx->vgacntrl.a, 0, 1<<31);

	/* turn off all pipes */
	for(x = 0; x < igfx->npipe; x++)
		disablepipe(igfx, x);

	/* check if enough memory has been mapped for requested mode */
	checkgtt(igfx, vga->mode);

	if(igfx->type == TypeG45){
		/* toggle dsp a on and off (from enable sequence) */
		csr(igfx, igfx->pipe[0].conf.a, 3<<18, 0);
		csr(igfx, igfx->pipe[0].dsp->cntr.a, 0, 1<<31);
		wr(igfx, igfx->pipe[0].dsp->surf.a, 0);		/* arm */
		csr(igfx, igfx->pipe[0].dsp->cntr.a, 1<<31, 0);
		wr(igfx, igfx->pipe[0].dsp->surf.a, 0);		/* arm */
		csr(igfx, igfx->pipe[0].conf.a, 0, 3<<18);
	}

	if(igfx->type == TypeHSW){
		/* deselect port clock */
		for(x=0; x<nelem(igfx->dpllsel); x++)
			csr(igfx, igfx->dpllsel[x].a, 0, 7<<29);

		/* disable dpll's other than LCPLL */
		for(x=1; x<nelem(igfx->dpll); x++)
			csr(igfx, igfx->dpll[x].ctrl.a, 1<<31, 0);
	}

	/* program new clock sources */
	loadreg(igfx, igfx->rawclkfreq);
	loadreg(igfx, igfx->drefctl);
	/* program cpu pll */
	if(igfx->type == TypeHSW)
		for(x=0; x<nelem(igfx->dpll); x++)
			loadreg(igfx, igfx->dpll[x].ctrl);
	sleep(10);

	/* set lvds before enabling dpll */
	loadreg(igfx, igfx->lvds);

	/* new dpll setting */
	for(x=0; x<nelem(igfx->dpllsel); x++)
		loadreg(igfx, igfx->dpllsel[x]);

	/* program all pipes */
	for(x = 0; x < igfx->npipe; x++)
		enablepipe(igfx, x);

	/* program vga plane */
	loadreg(igfx, igfx->vgacntrl);

	/* program ports */
	loadreg(igfx, igfx->adpa);
	loadreg(igfx, igfx->sdvob);
	loadreg(igfx, igfx->sdvoc);
	for(x = 0; x < nelem(igfx->dp); x++){
		for(y=0; y<nelem(igfx->dp[x].buftrans); y++)
			loadreg(igfx, igfx->dp[x].buftrans[y]);
		loadreg(igfx, igfx->dp[x].bufctl);
		if(enabledp(igfx, &igfx->dp[x]) < 0)
			ctlr->flag |= Ferror;
	}

	/* program lcd power */
	loadreg(igfx, igfx->ppcontrol);

	ctlr->flag |= Fload;
}

static void
dumpreg(char *name, char *item, Reg r)
{
	if(r.a == 0)
		return;

	printitem(name, item);
	Bprint(&stdout, " [%.8ux] = %.8ux\n", r.a, r.v);
}

static void
dumptiming(char *name, Trans *t)
{
	int tu, m, n;

	if(t->dm[0].a != 0 && t->dm[0].v != 0){
		tu = (t->dm[0].v >> 25)+1;
		printitem(name, "dm1 tu");
		Bprint(&stdout, " %d\n", tu);

		m = t->dm[0].v & 0xffffff;
		n = t->dn[0].v;
		if(n > 0){
			printitem(name, "dm1/dn1");
			Bprint(&stdout, " %f\n", (double)m / (double)n);
		}

		m = t->lm[0].v;
		n = t->ln[0].v;
		if(n > 0){
			printitem(name, "lm1/ln1");
			Bprint(&stdout, " %f\n", (double)m / (double)n);
		}
	}
}

static void
dumptrans(char *name, Trans *t)
{
	dumpreg(name, "conf", t->conf);

	dumpreg(name, "dm1", t->dm[0]);
	dumpreg(name, "dn1", t->dn[0]);
	dumpreg(name, "lm1", t->lm[0]);
	dumpreg(name, "ln1", t->ln[0]);
	dumpreg(name, "dm2", t->dm[1]);
	dumpreg(name, "dn2", t->dn[1]);
	dumpreg(name, "lm2", t->lm[1]);
	dumpreg(name, "ln2", t->ln[1]);

	dumptiming(name, t);

	dumpreg(name, "ht", t->ht);
	dumpreg(name, "hb", t->hb);
	dumpreg(name, "hs", t->hs);

	dumpreg(name, "vt", t->vt);
	dumpreg(name, "vb", t->vb);
	dumpreg(name, "vs", t->vs);
	dumpreg(name, "vss", t->vss);

	dumpreg(name, "dpctl", t->dpctl);
	dumpreg(name, "clksel", t->clksel);
}

static void
dumppipe(Igfx *igfx, int x)
{
	char name[32];
	Pipe *p;

	p = &igfx->pipe[x];

	snprint(name, sizeof(name), "%s pipe %c", igfx->ctlr->name, 'a'+x);
	dumpreg(name, "src", p->src);
	dumptrans(name, p);

	snprint(name, sizeof(name), "%s fdi %c", igfx->ctlr->name, 'a'+x);
	dumptrans(name, p->fdi);
	dumpreg(name, "txctl", p->fdi->txctl);
	dumpreg(name, "rxctl", p->fdi->rxctl);
	dumpreg(name, "rxmisc", p->fdi->rxmisc);
	dumpreg(name, "rxtu1", p->fdi->rxtu[0]);
	dumpreg(name, "rxtu2", p->fdi->rxtu[1]);
	dumpreg(name, "dpctl", p->fdi->dpctl);

	snprint(name, sizeof(name), "%s dsp %c", igfx->ctlr->name, 'a'+x);
	dumpreg(name, "cntr", p->dsp->cntr);
	dumpreg(name, "linoff", p->dsp->linoff);
	dumpreg(name, "stride", p->dsp->stride);
	dumpreg(name, "surf", p->dsp->surf);
	dumpreg(name, "tileoff", p->dsp->tileoff);
	dumpreg(name, "leftsurf", p->dsp->leftsurf);
	dumpreg(name, "pos", p->dsp->pos);
	dumpreg(name, "size", p->dsp->size);

	snprint(name, sizeof(name), "%s cur %c", igfx->ctlr->name, 'a'+x);
	dumpreg(name, "cntr", p->cur->cntr);
	dumpreg(name, "base", p->cur->base);
	dumpreg(name, "pos", p->cur->pos);
}

static void
dumpdpll(Igfx *igfx, int x)
{
	int cref, m1, m2, n, p1, p2;
	uvlong freq;
	char name[32];
	Dpll *dpll;
	u32int m;

	dpll = &igfx->dpll[x];
	snprint(name, sizeof(name), "%s dpll %c", igfx->ctlr->name, 'a'+x);

	dumpreg(name, "ctrl", dpll->ctrl);
	dumpreg(name, "fp0", dpll->fp0);
	dumpreg(name, "fp1", dpll->fp1);

	if(igfx->type == TypeHSW)
		return;

	p2 = ((dpll->ctrl.v >> 13) & 3) == 3 ? 14 : 10;
	if(((dpll->ctrl.v >> 24) & 3) == 1)
		p2 >>= 1;
	m = (dpll->ctrl.v >> 16) & 0xFF;
	for(p1 = 1; p1 <= 8; p1++)
		if(m & (1<<(p1-1)))
			break;
	printitem(name, "ctrl p1");
	Bprint(&stdout, " %d\n", p1);
	printitem(name, "ctrl p2");
	Bprint(&stdout, " %d\n", p2);

	n = (dpll->fp0.v >> 16) & 0x3f;
	m1 = (dpll->fp0.v >> 8) & 0x3f;
	m2 = (dpll->fp0.v >> 0) & 0x3f;

	cref = getcref(igfx, x);
	freq = ((uvlong)cref * (5*(m1+2) + (m2+2)) / (n+2)) / (p1 * p2);

	printitem(name, "fp0 m1");
	Bprint(&stdout, " %d\n", m1);
	printitem(name, "fp0 m2");
	Bprint(&stdout, " %d\n", m2);
	printitem(name, "fp0 n");
	Bprint(&stdout, " %d\n", n);

	printitem(name, "cref");
	Bprint(&stdout, " %d\n", cref);
	printitem(name, "fp0 freq");
	Bprint(&stdout, " %lld\n", freq);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	char name[32];
	Igfx *igfx;
	int x, y;

	if((igfx = vga->private) == nil)
		return;

	for(x=0; x<igfx->npipe; x++)
		dumppipe(igfx, x);

	for(x=0; x<nelem(igfx->dpll); x++){
		snprint(name, sizeof(name), "%s dpll %c", ctlr->name, 'a'+x);
		dumpreg(name, "ctrl", igfx->dpll[x].ctrl);
		dumpreg(name, "fp0", igfx->dpll[x].fp0);
		dumpreg(name, "fp1", igfx->dpll[x].fp1);
	}

	for(x=0; x<nelem(igfx->dpllsel); x++){
		snprint(name, sizeof(name), "%s dpllsel %c", ctlr->name, 'a'+x);
		dumpreg(name, "dpllsel", igfx->dpllsel[x]);
	}

	dumpreg(ctlr->name, "drefctl", igfx->drefctl);
	dumpreg(ctlr->name, "rawclkfreq", igfx->rawclkfreq);
	dumpreg(ctlr->name, "ssc4params", igfx->ssc4params);

	for(x=0; x<nelem(igfx->dp); x++){
		snprint(name, sizeof(name), "%s dp %c", ctlr->name, 'a'+x);
		dumpreg(name, "ctl", igfx->dp[x].ctl);
		dumpreg(name, "bufctl", igfx->dp[x].bufctl);
		dumpreg(name, "stat", igfx->dp[x].stat);
		dumphex(name, "dpcd", igfx->dp[x].dpcd, sizeof(igfx->dp[x].dpcd));
		for(y=0; y<nelem(igfx->dp[x].buftrans); y++){
			snprint(name, sizeof(name), "%s buftrans %c %d", ctlr->name, 'a'+x, y);
			dumpreg(name, "ctl", igfx->dp[x].buftrans[y]);
		}
	}
	for(x=0; x<nelem(igfx->hdmi); x++){
		snprint(name, sizeof(name), "%s hdmi %c", ctlr->name, 'a'+x);
		dumpreg(name, "ctl", igfx->hdmi[x].ctl);
	}

	for(x=0; x<nelem(igfx->pfit); x++){
		snprint(name, sizeof(name), "%s pfit %c", ctlr->name, 'a'+x);
		dumpreg(name, "ctrl", igfx->pfit[x].ctrl);
		dumpreg(name, "winpos", igfx->pfit[x].winpos);
		dumpreg(name, "winsize", igfx->pfit[x].winsize);
		dumpreg(name, "pwrgate", igfx->pfit[x].pwrgate);
	}

	dumpreg(ctlr->name, "ppcontrol", igfx->ppcontrol);
	dumpreg(ctlr->name, "ppstatus", igfx->ppstatus);

	dumpreg(ctlr->name, "adpa", igfx->adpa);
	dumpreg(ctlr->name, "lvds", igfx->lvds);
	dumpreg(ctlr->name, "sdvob", igfx->sdvob);
	dumpreg(ctlr->name, "sdvoc", igfx->sdvoc);

	dumpreg(ctlr->name, "vgacntrl", igfx->vgacntrl);
}

static int
dpauxio(Igfx *igfx, Dp *dp, uchar buf[20], int len)
{
	int t, i;
	u32int w;

	if(dp->auxctl.a == 0){
		werrstr("not present");
		return -1;
	}

	t = 0;
	while(rr(igfx, dp->auxctl.a) & (1<<31)){
		if(++t >= 10){
			werrstr("busy");
			return -1;
		}
		sleep(5);
	}

	/* clear sticky bits */
	wr(igfx, dp->auxctl.a, (1<<28) | (1<<25) | (1<<30));

	for(i=0; i<nelem(dp->auxdat); i++){
		w  = buf[i*4+0]<<24;
		w |= buf[i*4+1]<<16;
		w |= buf[i*4+2]<<8;
		w |= buf[i*4+3];
		wr(igfx, dp->auxdat[i].a, w);
	}

	/* 2X Bit Clock divider */
	w = ((dp == &igfx->dp[0]) ? igfx->cdclk : (igfx->rawclkfreq.v & 0x3ff)) >> 1;
	if(w < 1 || w > 0x3fd){
		werrstr("bad clock");
		return -1;
	}

	/* hack: slow down a bit */
	w += 2;

	w |= 1<<31;	/* SendBusy */
	w |= 1<<29;	/* interrupt disabled */
	w |= 3<<26;	/* timeout 1600µs */
	w |= len<<20;	/* send bytes */
	w |= 5<<16;	/* precharge time (5*2 = 10µs) */
	wr(igfx, dp->auxctl.a, w);

	t = 0;
	for(;;){
		w = rr(igfx, dp->auxctl.a);
		if((w & (1<<30)) != 0)
			break;
		if(++t >= 10){
			werrstr("busy");
			return -1;
		}
		sleep(5);
	}
	if(w & (1<<28)){
		werrstr("receive timeout");
		return -1;
	}
	if(w & (1<<25)){
		werrstr("receive error");
		return -1;
	}

	len = (w >> 20) & 0x1f;
	for(i=0; i<nelem(dp->auxdat); i++){
		w = rr(igfx, dp->auxdat[i].a);
		buf[i*4+0] = w>>24;
		buf[i*4+1] = w>>16;
		buf[i*4+2] = w>>8;
		buf[i*4+3] = w;
	}

	return len;
}

enum {
	CmdNative	= 8,
	CmdMot		= 4,
	CmdRead		= 1,
	CmdWrite	= 0,
};

static int
dpauxtra(Igfx *igfx, Dp *dp, int cmd, int addr, uchar *data, int len)
{
	uchar buf[20];
	int r;

	assert(len <= 16);

	memset(buf, 0, sizeof(buf));
	buf[0] = (cmd << 4) | ((addr >> 16) & 0xF);
	buf[1] = addr >> 8;
	buf[2] = addr;
	buf[3] = len-1;
	r = 3;	
	if(data != nil && len > 0){
		if((cmd & CmdRead) == 0)
			memmove(buf+4, data, len);
		r = 4;
		if((cmd & CmdRead) == 0)
			r += len;
	}
	if((r = dpauxio(igfx, dp, buf, r)) < 0){
		trace("%s: dpauxio: dp %c, cmd %x, addr %x, len %d: %r\n",
			igfx->ctlr->name, 'a'+(int)(dp - &igfx->dp[0]), cmd, addr, len);
		return -1;
	}
	if(r == 0 || data == nil || len == 0)
		return 0;
	if((cmd & CmdRead) != 0){
		if(--r < len)
			len = r;
		memmove(data, buf+1, len);
	}
	return len;
}

static int
rdpaux(Igfx *igfx, Dp *dp, int addr)
{
	uchar buf[1];
	if(dpauxtra(igfx, dp, CmdNative|CmdRead, addr, buf, 1) != 1)
		return -1;
	return buf[0];
}
static int
wdpaux(Igfx *igfx, Dp *dp, int addr, uchar val)
{
	if(dpauxtra(igfx, dp, CmdNative|CmdWrite, addr, &val, 1) != 1)
		return -1;
	return 0;
}

static int
enabledp(Igfx *igfx, Dp *dp)
{
	int try, r;
	u32int w;

	if(dp->ctl.a == 0)
		return 0;
	if((dp->ctl.v & (1<<31)) == 0)
		return 0;

	/* FIXME: always times out */
	if(igfx->type == TypeHSW && dp == &igfx->dp[0])
		goto Skip;

	/* Link configuration */
	wdpaux(igfx, dp, 0x100, (270*MHz) / 27000000);
	w = dp->ctl.v >> (igfx->type == TypeHSW ? 1 : 19) & 7;
	wdpaux(igfx, dp, 0x101, w+1);

	r = 0;

	/* Link training pattern 1 */
	dp->ctl.v &= ~(7<<8);
	loadreg(igfx, dp->ctl);
	for(try = 0;;try++){
		if(try > 5)
			goto Fail;
		/* Link training pattern 1 */
		wdpaux(igfx, dp, 0x102, 0x01);
		sleep(100);
		if((r = rdpaux(igfx, dp, 0x202)) < 0)
			goto Fail;
		if(r & 1)	/* LANE0_CR_DONE */
			break;
	}
	trace("pattern1 finished: %x\n", r);

	/* Link training pattern 2 */
	dp->ctl.v &= ~(7<<8);
	dp->ctl.v |= 1<<8;
	loadreg(igfx, dp->ctl);
	for(try = 0;;try++){
		if(try > 5)
			goto Fail;
		/* Link training pattern 2 */
		wdpaux(igfx, dp, 0x102, 0x02);
		sleep(100);
		if((r = rdpaux(igfx, dp, 0x202)) < 0)
			goto Fail;
		if((r & 7) == 7)
			break;
	}
	trace("pattern2 finished: %x\n", r);

	if(igfx->type == TypeHSW){
		/* set link training to idle pattern and wait for 5 idle
		 * patterns */
		dp->ctl.v &= ~(7<<8);
		dp->ctl.v |= 2<<8;
		loadreg(igfx, dp->ctl);
		for(try=0; try<10; try++){
			sleep(10);
			if(rr(igfx, dp->stat.a) & (1<<25))
				break;
		}
	}
Skip:
	/* stop training */
	dp->ctl.v &= ~(7<<8);
	dp->ctl.v |= 3<<8;
	loadreg(igfx, dp->ctl);
	wdpaux(igfx, dp, 0x102, 0x00);
	return 1;

Fail:
	trace("training failed: %x\n", r);

	/* disable port */
	dp->ctl.v &= ~(1<<31);
	loadreg(igfx, dp->ctl);
	wdpaux(igfx, dp, 0x102, 0x00);
	return -1;
}

static uchar*
edidshift(uchar buf[256])
{
	uchar tmp[256];
	int i;

	/* shift if neccesary so edid block is at the start */
	for(i=0; i<256-8; i++){
		if(buf[i+0] == 0x00 && buf[i+1] == 0xFF && buf[i+2] == 0xFF && buf[i+3] == 0xFF
		&& buf[i+4] == 0xFF && buf[i+5] == 0xFF && buf[i+6] == 0xFF && buf[i+7] == 0x00){
			memmove(tmp, buf, i);
			memmove(buf, buf + i, 256 - i);
			memmove(buf + (256 - i), tmp, i);
			break;
		}
	}
	return buf;
}

static Edid*
snarfdpedid(Igfx *igfx, Dp *dp, int addr)
{
	uchar buf[256];
	int i;
	Edid *e;

	for(i=0; i<sizeof(dp->dpcd); i+=16)
		if(dpauxtra(igfx, dp, CmdNative|CmdRead, i, dp->dpcd+i, 16) != 16)
			return nil;

	if(dp->dpcd[0] == 0)	/* nothing there, dont try to get edid */
		return nil;

	if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, nil, 0) < 0)
		return nil;

	for(i=0; i<sizeof(buf); i+=16){
		if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, buf+i, 16) == 16)
			continue;
		if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, buf+i, 16) == 16)
			continue;
		if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, buf+i, 16) == 16)
			continue;
		if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, buf+i, 16) == 16)
			continue;
		if(dpauxtra(igfx, dp, CmdMot|CmdRead, addr, buf+i, 16) == 16)
			continue;
		return nil;
	}

	dpauxtra(igfx, dp, CmdRead, addr, nil, 0);

	if((e = parseedid128(edidshift(buf))) == nil)
		trace("%s: snarfdpedid: dp %c: %r\n", igfx->ctlr->name, 'a'+(int)(dp - &igfx->dp[0]));
	return e;
}

enum {
	GMBUSCP = 0,	/* Clock/Port selection */
	GMBUSCS = 1,	/* Command/Status */
	GMBUSST = 2,	/* Status Register */
	GMBUSDB	= 3,	/* Data Buffer Register */
	GMBUSIM = 4,	/* Interrupt Mask */
	GMBUSIX = 5,	/* Index Register */
};
	
static int
gmbusread(Igfx *igfx, int port, int addr, uchar *data, int len)
{
	u32int x, y;
	int n, t;

	if(igfx->gmbus[GMBUSCP].a == 0)
		return -1;

	wr(igfx, igfx->gmbus[GMBUSCP].a, port);
	wr(igfx, igfx->gmbus[GMBUSIX].a, 0);

	/* bus cycle without index and stop, byte count, slave address, read */
	wr(igfx, igfx->gmbus[GMBUSCS].a, 1<<30 | 5<<25 | len<<16 | addr<<1 | 1);

	n = 0;
	while(len > 0){
		x = 0;
		for(t=0; t<100; t++){
			x = rr(igfx, igfx->gmbus[GMBUSST].a);
			if(x & (1<<11))
				break;
			sleep(5);
		}
		if((x & (1<<11)) == 0)
			return -1;

		t = 4 - (x & 3);
		if(t > len)
			t = len;
		len -= t;

		y = rr(igfx, igfx->gmbus[GMBUSDB].a);
		switch(t){
		case 4:
			data[n++] = y & 0xff, y >>= 8;
		case 3:
			data[n++] = y & 0xff, y >>= 8;
		case 2:
			data[n++] = y & 0xff, y >>= 8;
		case 1:
			data[n++] = y & 0xff;
		}
	}

	return n;
}

static Edid*
snarfgmedid(Igfx *igfx, int port, int addr)
{
	uchar buf[256];

	/* read twice */
	if(gmbusread(igfx, port, addr, buf, 128) != 128)
		return nil;
	if(gmbusread(igfx, port, addr, buf + 128, 128) != 128)
		return nil;

	return parseedid128(edidshift(buf));
}

Ctlr igfx = {
	"igfx",			/* name */
	snarf,			/* snarf */
	options,		/* options */
	init,			/* init */
	load,			/* load */
	dump,			/* dump */
};

Ctlr igfxhwgc = {
	"igfxhwgc",
};
