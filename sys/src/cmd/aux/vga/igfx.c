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
	TypeIVB,		/* Ivy Bdige */
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
};

struct Hdmi {
	Reg	ctl;
	Reg	bufctl[4];
};

struct Dp {
	Reg	ctl;
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
};

struct Curs {
	Reg	cntr;
	Reg	base;
	Reg	pos;
};

struct Pipe {
	Trans;

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

	int	type;

	int	npipe;
	Pipe	pipe[4];

	Dpll	dpll[2];
	Pfit	pfit[3];

	/* IVB */
	Reg	dpllsel;	/* DPLL_SEL */
	Reg	drefctl;	/* DREF_CTL */
	Reg	rawclkfreq;	/* RAWCLK_FREQ */
	Reg	ssc4params;	/* SSC4_PARAMS */

	Dp	dp[4];
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
	outportl(igfx->pio, a);
	return inportl(igfx->pio + 4);
}
static void
wr(Igfx *igfx, u32int a, u32int v)
{
	if(a == 0)	/* invalid */
		return;

	assert((a & 3) == 0);
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
	}
}

static void
snarfpipe(Igfx *igfx, int x)
{
	u32int o;
	Pipe *p;

	p = &igfx->pipe[x];

	o = 0x60000 | x*0x1000;
	snarftrans(igfx, p, o);

	p->src = snarfreg(igfx, o + 0x0001C);

	if(igfx->type == TypeIVB) {
		p->fdi->txctl = snarfreg(igfx, o + 0x100);

		o = 0xE0000 | x*0x1000;
		snarftrans(igfx, p->fdi, o);

		p->fdi->dpctl = snarfreg(igfx, o + 0x300);

		p->fdi->rxctl = snarfreg(igfx, o + 0x1000c);
		p->fdi->rxmisc = snarfreg(igfx, o + 0x10010);
		p->fdi->rxtu[0] = snarfreg(igfx, o + 0x10030);
		p->fdi->rxtu[1] = snarfreg(igfx, o + 0x10038);

		p->fdi->chicken = snarfreg(igfx, o + 0x10064);

		p->fdi->dpll = &igfx->dpll[(igfx->dpllsel.v>>(x*4)) & 1];
		p->dpll = nil;
	} else {
		p->dpll = &igfx->dpll[x & 1];
	}

	/* display plane */
	p->dsp->cntr		= snarfreg(igfx, 0x70180 | x*0x1000);
	p->dsp->linoff		= snarfreg(igfx, 0x70184 | x*0x1000);
	p->dsp->stride		= snarfreg(igfx, 0x70188 | x*0x1000);
	p->dsp->tileoff		= snarfreg(igfx, 0x701A4 | x*0x1000);
	p->dsp->surf		= snarfreg(igfx, 0x7019C | x*0x1000);

	/* cursor plane */
	switch(igfx->type){
	case TypeIVB:
		p->cur->cntr	= snarfreg(igfx, 0x70080 | x*0x1000);
		p->cur->base	= snarfreg(igfx, 0x70084 | x*0x1000);
		p->cur->pos	= snarfreg(igfx, 0x70088 | x*0x1000);
		break;
	case TypeG45:
		p->cur->cntr	= snarfreg(igfx, 0x70080 | x*0x40);
		p->cur->base	= snarfreg(igfx, 0x70084 | x*0x40);
		p->cur->pos	= snarfreg(igfx, 0x7008C | x*0x40);
		break;
	}
}

static int
devtype(Igfx *igfx)
{
	if(igfx->pci->vid != 0x8086)
		return -1;
	switch(igfx->pci->did){
	case 0x0166:	/* X230 */
		return TypeIVB;

	case 0x2a42:	/* X200s */
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
		if((igfx->pci->mem[4].bar & 1) == 0){
			error("%s: no pio bar\n", ctlr->name);
			return;
		}
		igfx->pio = igfx->pci->mem[4].bar & ~1;
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

	case TypeIVB:
		igfx->npipe = 3;	/* A,B,C */

		igfx->dpll[0].ctrl	= snarfreg(igfx, 0xC6014);
		igfx->dpll[0].fp0	= snarfreg(igfx, 0xC6040);
		igfx->dpll[0].fp1	= snarfreg(igfx, 0xC6044);
		igfx->dpll[1].ctrl	= snarfreg(igfx, 0xC6018);
		igfx->dpll[1].fp0	= snarfreg(igfx, 0xC6048);
		igfx->dpll[1].fp1	= snarfreg(igfx, 0xC604c);

		igfx->dpllsel		= snarfreg(igfx, 0xC7000);

		igfx->drefctl		= snarfreg(igfx, 0xC6200);
		igfx->rawclkfreq	= snarfreg(igfx, 0xC6204);
		igfx->ssc4params	= snarfreg(igfx, 0xC6210);

		/* cpu displayport A*/
		igfx->dp[0].ctl		= snarfreg(igfx, 0x64000);
		igfx->dp[0].auxctl	= snarfreg(igfx, 0x64010);
		igfx->dp[0].auxdat[0]	= snarfreg(igfx, 0x64014);
		igfx->dp[0].auxdat[1]	= snarfreg(igfx, 0x64018);
		igfx->dp[0].auxdat[2]	= snarfreg(igfx, 0x6401C);
		igfx->dp[0].auxdat[3]	= snarfreg(igfx, 0x64020);
		igfx->dp[0].auxdat[4]	= snarfreg(igfx, 0x64024);

		/* pch displayport B,C,D */
		for(x=1; x<4; x++){
			igfx->dp[x].ctl		= snarfreg(igfx, 0xE4000 + 0x100*x);
			igfx->dp[x].auxctl	= snarfreg(igfx, 0xE4010 + 0x100*x);
			igfx->dp[x].auxdat[0]	= snarfreg(igfx, 0xE4014 + 0x100*x);
			igfx->dp[x].auxdat[1]	= snarfreg(igfx, 0xE4018 + 0x100*x);
			igfx->dp[x].auxdat[2]	= snarfreg(igfx, 0xE401C + 0x100*x);
			igfx->dp[x].auxdat[3]	= snarfreg(igfx, 0xE4020 + 0x100*x);
			igfx->dp[x].auxdat[4]	= snarfreg(igfx, 0xE4024 + 0x100*x);
		}

		for(x=0; x<3; x++){
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

		igfx->adpa		= snarfreg(igfx, 0x0E1100);	/* DAC_CTL */
		igfx->lvds		= snarfreg(igfx, 0x0E1180);	/* LVDS_CTL */

		igfx->vgacntrl		= snarfreg(igfx, 0x041000);
		break;
	}

	for(x=0; x<igfx->npipe; x++)
		snarfpipe(igfx, x);

	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	USED(vga);
	ctlr->flag |= Hlinear|Ulinear|Foptions;
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
	int cref, m1, m2, n, p1, p2;
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
		igfx->dpllsel.v |= 8<<(x*4);
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

	dpll->fp1.v = dpll->fp0.v;

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
initpipe(Pipe *p, Mode *m)
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
	}

	/* bits per color for cpu pipe */
	for(i=0; i<nelem(bpctab); i++){
		if(bpctab[i] == bpc){
			p->conf.v &= ~(7<<5);
			p->conf.v |= i<<5;
			break;
		}
	}
	initdatalinkmn(p, m->frequency, 270*MHz, lanes, tu, 3*bpc);
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	int x, islvds;
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

	x = 0;
	islvds = 0;
	if((val = dbattr(m->attr, "lcd")) != nil && atoi(val) != 0){
		islvds = 1;

		if(igfx->npipe > 2)
			x = (igfx->lvds.v >> 29) & 3;
		else
			x = (igfx->lvds.v >> 30) & 1;
		igfx->lvds.v |= (1<<31);

		igfx->ppcontrol.v &= ~0xFFFF0000;
		igfx->ppcontrol.v |= 5;
	}
	p = &igfx->pipe[x];

	/* plane enable, 32bpp and assign pipe */
	p->dsp->cntr.v = (1<<31) | (6<<26) | (x<<24);

	/* stride must be 64 byte aligned */
	p->dsp->stride.v = m->x * (m->z / 8);
	p->dsp->stride.v += 63;
	p->dsp->stride.v &= ~63;

	/* virtual width in pixels */
	vga->virtx = p->dsp->stride.v / (m->z / 8);

	p->dsp->surf.v = 0;
	p->dsp->linoff.v = 0;
	p->dsp->tileoff.v = 0;

	/* cursor plane off */
	p->cur->cntr.v = x<<28;
	p->cur->pos.v = 0;
	p->cur->base.v = 0;

	if(initdpll(igfx, x, m->frequency, islvds, 0) < 0)
		error("%s: frequency %d out of range\n", ctlr->name, m->frequency);

	initpipe(p, m);

	/*
	 * undocumented magic that makes the flickering
	 * top bar go away on x230 on lcd. found by
	 * comparing registers set by vesa bios.
	 */
	if(igfx->type == TypeIVB && islvds)
		p->conf.v |= 3<<27;

	ctlr->flag |= Finit;
}

static void
loadtrans(Igfx *igfx, Trans *t)
{
	int i;

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

	if(t->dpll != nil){
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

	if(0){
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

	/* the front fell off on x230 */
	if(igfx->type == TypeIVB && t == &igfx->pipe[0])
		goto skippipe;

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
	if(t->dpll != nil)
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
	csr(igfx, p->cur->cntr.a, 1<<5 | 7, 0);
	csr(igfx, p->cur->base.a, ~0, 0);	/* arm */

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
	csr(igfx, igfx->dpllsel.a, 8<<(x*4), 0);
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	Igfx *igfx;
	int x;

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
	for(x = 0; x < nelem(igfx->dp); x++)
		csr(igfx, igfx->dp[x].ctl.a, 1<<31, 0);
	for(x = 0; x < nelem(igfx->hdmi); x++)
		csr(igfx, igfx->hdmi[x].ctl.a, 1<<31, 0);

	/* disable vga plane */
	csr(igfx, igfx->vgacntrl.a, 0, 1<<31);

	/* turn off all pipes */
	for(x = 0; x < igfx->npipe; x++)
		disablepipe(igfx, x);

	/* program new clock sources */
	loadreg(igfx, igfx->rawclkfreq);
	loadreg(igfx, igfx->drefctl);
	sleep(10);

	/* set lvds before enabling dpll */
	loadreg(igfx, igfx->lvds);

	/* new dpll setting */
	loadreg(igfx, igfx->dpllsel);

	/* program all pipes */
	for(x = 0; x < igfx->npipe; x++)
		enablepipe(igfx, x);

	/* program vga plane */
	loadreg(igfx, igfx->vgacntrl);

	/* program ports */
	loadreg(igfx, igfx->adpa);
	loadreg(igfx, igfx->sdvob);
	loadreg(igfx, igfx->sdvoc);
	for(x = 0; x < nelem(igfx->dp); x++)
		loadreg(igfx, igfx->dp[x].ctl);
	for(x=0; x<nelem(igfx->hdmi); x++){
		loadreg(igfx, igfx->hdmi[x].bufctl[0]);
		loadreg(igfx, igfx->hdmi[x].bufctl[1]);
		loadreg(igfx, igfx->hdmi[x].bufctl[2]);
		loadreg(igfx, igfx->hdmi[x].bufctl[3]);
		loadreg(igfx, igfx->hdmi[x].ctl);
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

	snprint(name, sizeof(name), "%s cur %c", igfx->ctlr->name, 'a'+x);
	dumpreg(name, "cntr", p->cur->cntr);
	dumpreg(name, "base", p->cur->base);
	dumpreg(name, "pos", p->cur->pos);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	char name[32];
	Igfx *igfx;
	int x;

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

	dumpreg(ctlr->name, "dpllsel", igfx->dpllsel);

	dumpreg(ctlr->name, "drefctl", igfx->drefctl);
	dumpreg(ctlr->name, "rawclkfreq", igfx->rawclkfreq);
	dumpreg(ctlr->name, "ssc4params", igfx->ssc4params);

	for(x=0; x<nelem(igfx->dp); x++){
		snprint(name, sizeof(name), "%s dp %c", ctlr->name, 'a'+x);
		dumpreg(name, "ctl", igfx->dp[x].ctl);
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
