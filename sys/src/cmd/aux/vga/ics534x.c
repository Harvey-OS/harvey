#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ICS534x GENDAC.
 * For now assumed to be hooked to either a Tseng Labs ET4000-W32p
 * (Hercules Dynamite Power, the Hercules generates RS2 using CLK3)
 * or an ARK2000pv (Diamond Stealth64 Graphics 2001).
 */
static uchar
setrs2(Vga* vga, Ctlr* ctlr)
{
	uchar rs2;

	rs2 = 0;
	if(strncmp(vga->ctlr->name, "et4000-w32", 10) == 0){
		rs2 = vgaxi(Crtx, 0x31);
		vgaxo(Crtx, 0x31, 0x40|rs2);
	}
	else if(strncmp(vga->ctlr->name, "ark2000pv", 9) == 0){
		rs2 = vgaxi(Seqx, 0x1C);
		vgaxo(Seqx, 0x1C, 0x80|rs2);
	}
	else
		error("%s: not configured for %s\n", vga->ctlr->name, ctlr->name);

	return rs2;
}

static void
restorers2(Vga* vga, uchar rs2)
{
	if(strncmp(vga->ctlr->name, "et4000-w32", 10) == 0)
		vgaxo(Crtx, 0x31, rs2);
	else if(strncmp(vga->ctlr->name, "ark2000pv", 9) == 0)
		vgaxo(Seqx, 0x1C, rs2);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hpclk2x8|Foptions;
}

static void
clock(Vga* vga, Ctlr* ctlr)
{
	ulong f, m, n, r;
	double fmin, fmax, t, tok;

	/*
	 * The PLL frequency is defined by:
	 *		    (M+2)
	 *	 Fout = ------------ x Fref
	 *	        (N+2) x 2**R
	 * where M, N and R have the following contraints:
	 * 1)	     2MHz < Fref < 32MHz
	 * 2)		    Fref
	 *         600KHz < ----- <= 8Mhz
	 *	            (N+2)
	 * 3)		(M+2) x Fref
	 *     60MHz <= ------------ <= 270MHz
	 *		    (N+2)
	 * 4) Fout < 135MHz
	 * 5) 1 <= M <= 127, 1 <= N <= 31, 0 <= R <= 3
	 *
	 * First determine R by finding the highest value
	 * for which
	 *	      2**R x Fout <= 270Mhz
	 * The datasheet says that if the multiplexed 16-bit
	 * pseudo-colour mode is used then N2 (vga->r) must
	 * be 2.
	 */
	if(ctlr->flag & Upclk2x8)
		vga->r[0] = 2;
	else{
		vga->r[0] = 4;
		for(r = 0; r <= 3; r++){
			f = vga->f[0]*(1<<r);
			if(60000000 < f && f <= 270000000)
				vga->r[0] = r;
		}
		if(vga->r[0] > 3)
			error("%s: pclk %lud out of range\n",
				ctlr->name, vga->f[0]);
	}

	/*
	 * Now find the closest match for M and N.
	 * Lower values of M and N give better noise rejection.
	 */
	fmin = vga->f[0]*0.995;
	fmax = vga->f[0]*1.005;
	tok = 0.0;
	for(n = 31; n >= 1; n--){
		t = RefFreq/(n+2);
		if(600000 >= t || t > 8000000)
			continue;

		t = vga->f[0]*(n+2)*(1<<vga->r[0]);
		t /= RefFreq;
		m = (t+0.5) - 2;
		if(m > 127)
			continue;

		t = (m+2)*RefFreq;
		t /= (n+2)*(1<<vga->r[0]);

		if(fmin <= t && t < fmax){
			vga->m[0] = m;
			vga->n[0] = n;
			tok = t;
		}
	}

	if(tok == 0.0)
		error("%s: pclk %lud out of range\n", ctlr->name, vga->f[0]);
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong pclk;
	char *p;

	/*
	 * Part comes in -135, -110 and -80MHz speed-grades.
	 */
	pclk = 80000000;
	if(p = strrchr(ctlr->name, '-'))
		pclk = strtoul(p+1, 0, 0) * 1000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > pclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);

	/*
	 * Determine whether to use 2x8-bit mode or not.
	 * If yes and the clock has already been initialised,
	 * initialise it again.
	 */
	if(vga->ctlr && (vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8 && vga->f[0] >= pclk/2){
		vga->f[0] /= 2;
		resyncinit(vga, ctlr, Upclk2x8, 0);
	}

	/*
	 * Clock bits. If the desired video clock is
	 * one of the two standard VGA clocks it can just be
	 * set using bits <3:2> of vga->misc, otherwise we
	 * need to programme the DCLK PLL.
	 */
	vga->misc &= ~0x0C;
	if(vga->f[0] == VgaFreq0)
		vga->i[0] = 0;
	else if(vga->f[0] == VgaFreq1){
		vga->misc |= 0x04;
		vga->i[0] = 1;
	}
	else{
		/*
		 * Initialise the PLL parameters.
		 * Use CLK0 f7 internal clock (there are only 3
		 * clock-select bits).
		 */
		clock(vga, ctlr);
		vga->i[0] = 0x07;
	}

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar rs2, mode, pll;

	rs2 = setrs2(vga, ctlr);

	/*
	 * Put the chip into snooze mode,
	 * colour mode 0.
	 */
	mode = 0x00;
	outportb(Pixmask, 0x01);

	if(ctlr->flag & Upclk2x8)
		mode = 0x10;

	/*
	 * If necessary, set the PLL parameters for CLK0 f7
	 * and make sure the PLL control register selects the
	 * correct clock. Preserve the memory clock setting.
	 */
	outportb(PaddrR, 0x0E);
	pll = inportb(Pdata) & 0x10;
	if(vga->i[0] == 0x07){
		outportb(PaddrW, vga->i[0]);
		outportb(Pdata, vga->m[0]);
		outportb(Pdata, (vga->r[0]<<5)|vga->n[0]);
		pll |= 0x27;
	}
	outportb(PaddrW, 0x0E);
	outportb(Pdata, pll);
	outportb(Pixmask, mode);

	restorers2(vga, rs2);
	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	uchar rs2, m, n;
	char buf[32];
	ulong f;

	rs2 = setrs2(vga, ctlr);

	printitem(ctlr->name, "command");
	printreg(inportb(Pixmask));

	outportb(PaddrR, 0x00);
	for(i = 0; i < 0x0E; i++){
		sprint(buf, "f%X m n", i);
		printitem(ctlr->name, buf);
		m = inportb(Pdata);
		printreg(m);
		n = inportb(Pdata);
		printreg(n);

		f = 14318180*(m+2);
		f /= (n & 0x1F)+2;
		f /= 1<<((n>>5) & 0x03);
		Bprint(&stdout, "%12lud", f);
	}
	printitem(ctlr->name, "control");
	printreg(inportb(Pdata));

	restorers2(vga, rs2);
}

Ctlr ics534x = {
	"ics534x",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
