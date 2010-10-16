#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * S3 Trio/ViRGE/Savage4
 * Requires work.
 */

/* on some cards, savage4mem[1] == 8, but i don't know how to tell. -rsc */
static uchar savage4mem[] = { 2, 4, 8, 12, 16, 32, 0, 32};
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i, id;
	char *p;

	/*
	 * The Trio/ViRGE variants have some extra sequencer registers
	 * which need to be unlocked for access.
	 */
	vgaxo(Seqx, 0x08, 0x06);
	s3generic.snarf(vga, ctlr);

	for(i = 0x08; i < 0x4F; i++)
		vga->sequencer[i] = vgaxi(Seqx, i);
	vga->crt[0x2D] = vgaxi(Crtx, 0x2D);
	vga->crt[0x2E] = vgaxi(Crtx, 0x2E);
	vga->crt[0x2F] = vgaxi(Crtx, 0x2F);
	for(i = 0x70; i < 0x99; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	id = (vga->crt[0x2D]<<8)|vga->crt[0x2E];
	switch(id){

	default:
		trace("Unknown ViRGE/Trio64+ - 0x%4.4uX\n",
			(vga->crt[0x2D]<<8)|vga->crt[0x2E]);
		/*FALLTHROUGH*/

	case 0x8810:				/* Microsoft Virtual PC 2004 */
	case 0x8811:				/* Trio64+ */
		vga->r[1] = 3;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 135000000;
		trace("Trio64+\n");
		break;

	case 0x8812:				/* Aurora64V+ */
		vga->r[1] = 3;
		vga->m[1] = 127;
		vga->n[1] = 63;
		vga->f[1] = 135000000;
		trace("Aurora64V+\n");
		break;

	case 0x8901:				/* Trio64V2 */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 170000000;
		trace("Trio64V2\n");
		break;

	case 0x5631:				/* ViRGE */
		vga->r[1] = 3;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 135000000;
		vga->apz = 64*1024*1024;
		trace("ViRGE\n");
		break;

	case 0x8A01:				/* ViRGE/[DG]X */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 170000000;
		trace("ViRGE/[DG]X\n");
		break;

	case 0x8A10:				/* ViRGE/GX2 */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 170000000;
		vga->apz = 64*1024*1024;
		trace("ViRGE/GX2\n");
		/*
		 * Memory encoding on the ViRGE/GX2 is different.
		 */
		switch((vga->crt[0x36]>>6) & 0x03){

		case 0x01:
			vga->vmz = 4*1024*1024;
			break;

		case 0x03:
			vga->vmz = 2*1024*1024;
			break;
		}
		break;

	case 0x883D:				/* ViRGE/VX */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 31;
		vga->f[1] = 220000000;
		vga->apz = 64*1024*1024;
		trace("ViRGE/VX\n");
		/*
		 * Memory encoding on the ViRGE/VX is different.
		 */
		vga->vmz = (2*(((vga->crt[0x36]>>5) & 0x03)+1)) * 1*1024*1024;
		break;

	case 0x8C10:				/* Savage MX/MV */
	case 0x8C12:				/* Savage4/IX-MV */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 127;
		vga->f[1] = 135000000;		/* without clock-doubling */
		for(i = 0x50; i < 0x70; i++)
			vga->sequencer[i] = vgaxi(Seqx, i);
		vga->apz = 128*1024*1024;
		vga->vmz = savage4mem[vga->crt[0x36]>>5] * 1024 * 1024;
		trace("Savage4/IX-MV\n");
		break;

	case 0x8C2E:				/* SuperSavage/IXC16 */
		/*
		 * This is all guessed, from the Savage4/IX,
		 * inspection of the card (apz), and web pages
		 * talking about the chip (f[1]).  It seems to
		 * work on the IBM Thinkpad T23.
		 *
		 * XXX only 1024x768 works, not 1400x1050.
		 */
		vga->r[1] = 4;
		vga->m[1] = 127;
		vga->n[1] = 127;
		vga->f[1] = 135000000;		/* 300000000 after doubling? */
		for(i = 0x50; i < 0x70; i++)
			vga->sequencer[i] = vgaxi(Seqx, i);
		vga->apz = 64*1024*1024;
		vga->vmz = savage4mem[vga->crt[0x36]>>5] * 1024 * 1024;
		trace("SuperSavage/IXC16\n");
		break;

	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
	case 0x8D04:				/* ProSavage DDR, K.Okamoto */
		vga->r[1] = 4;
		vga->m[1] = 511;
		vga->n[1] = 127;
		vga->f[1] = 300000000;	

		vga->apz = 128*1024*1024;
		vga->vmz = savage4mem[vga->crt[0x36]>>5] * 1024 * 1024;
		trace("Savage4\n");
		break;
	}

	/*
	 * Work out the part speed-grade from name. Name can have,
	 * e.g. '-135' on the end for 135MHz part.
	 */
	if(p = strrchr(ctlr->name, '-'))
		vga->f[1] = strtoul(p+1, 0, 0) * 1000000;

	ctlr->flag |= Fsnarf;
}

static void
options(Vga *vga, Ctlr* ctlr)
{
	int id;

	id = (vga->crt[0x2D]<<8)|(vga->crt[0x2E]);
	switch(id){
	case 0x8C2E:				/* SuperSavage/IXC16 */
	case 0x8C10:				/* Savage MX/MV */
	case 0x8C12:				/* Savage4/IX-MV */
	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
	case 0x8D04:				/* ProSavabe DDR, K.Okamoto */
		/*
		 * Round up so stride is multiple of 16.
		 */
		if(vga->virtx%16)
			vga->virtx = (vga->virtx+15)&~15;
		break;
	}

	ctlr->flag |= Hlinear|Hpclk2x8|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	char *p, *val;
	Mode *mode;
	ulong pclk, x;
	int id, noclockset, width;

	id = (vga->crt[0x2D]<<8)|vga->crt[0x2E];
	mode = vga->mode;

	/*
	 * double 16bpp horizontal timings on ViRGE/[DG]X
	 * leaves overflow bit-fiddling to s3generic.init
	 */
	if((id == 0x8A01) && ((mode->z+7)/8 == 2)){
		resyncinit(vga, ctlr, Uenhanced, 0);
		vga->crt[0x00] = ((mode->ht * 2) >> 3) - 5;
		vga->crt[0x01] = ((mode->x * 2) >> 3) - 1;
		vga->crt[0x02] = ((mode->shb * 2) >> 3) - 1;
		x = (mode->ehb * 2) >> 3;
		vga->crt[0x03] = 0x80 | (x & 0x1F);
		vga->crt[0x04] = (mode->shs * 2) >> 3;
		vga->crt[0x05] = ((mode->ehs * 2) >> 3) & 0x1F;
		if(x & 0x20)
			vga->crt[0x05] |= 0x80;
	}
	s3generic.init(vga, ctlr);

	/*
	 * Work out the part speed-grade from name. Name can have,
	 * e.g. '-135' on the end for 135MHz part.
	 */
	noclockset = 0;
	if(p = strrchr(ctlr->name, '-'))
		vga->f[1] = strtoul(p+1, 0, 0) * 1000000;
	pclk = vga->f[1];

	if(vga->mode->z > 8)
		width = vga->virtx*(vga->mode->z/8);
	else
		width = vga->virtx*(8/vga->mode->z);
	
	switch(id){

	case 0x8810:				/* Microsoft Virtual PC 2004 */
	case 0x8811:				/* Trio64+ */
		/*
		 * Part comes in -135MHz speed grade. In 8-bit mode
		 * the maximum DCLK is 80MHz. In 2x8-bit mode the maximum
		 * DCLK is 135MHz using the internal clock doubler.
		 */
		if((ctlr->flag & Hpclk2x8) && vga->mode->z == 8){
			if(vga->f[0] > 80000000)
				ctlr->flag |= Upclk2x8;
		}
		else
			pclk = 80000000;

		vga->crt[0x67] &= ~0xF2;
		if(ctlr->flag & Upclk2x8){
			vga->sequencer[0x15] |= 0x10;
			vga->sequencer[0x18] |= 0x80;
			/*
			 * There's a little strip of the border
			 * appears on the left in resolutions
			 * 1280 and above if the 0x02 bit isn't
			 * set (when it appears on the right...).
			 */
			vga->crt[0x67] |= 0x10;
		}
	
		/*
		 * VLB address latch delay.
		 */
		if((vga->crt[0x36] & 0x03) == 0x01)
			vga->crt[0x58] &= ~0x08;

		/*
		 * Display memory access control.
		 */
		vga->crt[0x60] = 0xFF;

		if(vga->mode->z > 8)
			error("trio64: depth %d not supported\n", vga->mode->z);
		break;

	case 0x8901:				/* Trio64V2 */
		vga->crt[0x90] = 0;
		vga->crt[0x91] = 0;
		break;
	case 0x8A10:				/* ViRGE/GX2 */
		vga->crt[0x90] = 0;
	
		vga->crt[0x31] |= 0x08;

		vga->crt[0x13] = (width>>3) & 0xFF;
		vga->crt[0x51] &= ~0x30;
		vga->crt[0x51] |= (width>>7) & 0x30;

		/*
		 * Increase primary FIFO threshold
	 	 * to reduce flicker and tearing.
		 */
		vga->crt[0x85] = 0x0F;

		/*FALLTHROUGH*/
	case 0x5631:				/* ViRGE */
	case 0x8A01:				/* ViRGE/[DG]X */
		if(id == 0x8A01){
			x = mode->x * ((mode->z + 7) / 8);
			x = (x + 7) / 8;
			vga->crt[0x91] = x & 0xFF;
			vga->crt[0x90] = (x >> 8) | 0x80;
		}
	case 0x883D:				/* ViRGE/VX */
		vga->crt[0x60] &= 0x0F;
		/*FALLTHROUGH*/
	case 0x8812:				/* Aurora64V+ */
		if(id == 0x8812)		/* Aurora64V+ */
			noclockset = 1;
		vga->crt[0x65] = 0;
		vga->crt[0x66] = 0x89;
		vga->crt[0x67] = 0;

		if(id == 0x883D){		/* ViRGE/VX */
			/*
			 * Put the VRAM in 1-cycle EDO mode.  
			 * If it is not put in this mode, hardware acceleration
			 * will leave little turds on the screen when hwfill is used.
			 */
			vga->crt[0x36] &= ~0x0C;

			if(vga->mode->x > 800 && vga->mode->z == 8)
				vga->crt[0x67] = 0x10;
			else
				vga->crt[0x67] = 0;

	 		/*
			 * Adjustments to the generic settings:
			 *	heuristic fiddling.
			 *
			 * We used to set crt[0x66] to 0x89, but setting it
			 * to 0x90 seems to allow us (and more importantly the card)
			 * to access more than 2MB of memory.
			 */
			vga->crt[0x66] = 0x90;
			vga->crt[0x58] &= ~0x88;
			vga->crt[0x58] |= 0x40;
			if(vga->mode->x > 640 && vga->mode->z >= 8)
				vga->crt[0x63] |= 0x01;
			else
				vga->crt[0x63] &= ~0x01;
		}

		/*
		 * The high nibble is the mode; or'ing in 0x02 turns
		 * on support for gamma correction via the DACs, but I
		 * haven't figured out how to turn on the 8-bit DACs,
		 * so gamma correction stays off.
		 */
		switch(vga->mode->z){
		case 1:
		case 2:
		case 4:
		case 8:
		default:
			vga->crt[0x67] |= 0x00;
			break;
		case 15:
			vga->crt[0x67] |= 0x30;
			break;
		case 16:
			vga->crt[0x67] |= 0x50;
			break;
		case 24:
			if(id == 0x8A10)	/* GX2 has to be different */
				vga->crt[0x67] |= 0x70;
			else
				vga->crt[0x67] |= 0xD0;
			break;
		case 32:
			/*
			 * The ViRGE and ViRGE/VX manuals make no mention of
			 * 32-bit mode (which the GX/2 calls 24-bit unpacked mode).
			 */
			if(id != 0x8A10)
				error("32-bit mode only supported on the GX/2\n");
			vga->crt[0x67] |= 0xD0;
			break;
		}

		/*
		 * Set new MMIO method
		 */
		vga->crt[0x53] &= ~0x18;
		vga->crt[0x53] |= 0x08;

		break;

	case 0x8C2E:				/* SuperSavage/IXC16 (let's try this -rsc) */
	case 0x8C10:				/* Savage MX/MV */
	case 0x8C12:				/* Savage4/IX-MV */
		/*
		 * Experience shows the -1 (according to the manual)
		 * is incorrect.
		 */
		x = width/8 /*-1*/;
		vga->crt[0x91] = x;
		vga->crt[0x90] &= ~0x07;
		vga->crt[0x90] |= (x>>8) & 0x07;

	case 0x8D04:				/* ProSavage DDR, K.Okamoto */
		x = mode->x * ((mode->z + 7) / 8);
		x = (x + 7) / 8;
		vga->crt[0x91] = x & 0xFF;
		vga->crt[0x90] = (x >> 8) | 0x80;

		/*FALLTHROUGH*/

	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
		/*
		 * The Savage 4 is frustratingly similar to the
		 * ViRGE/GX2, but has enough slight differences
		 * to warrant special treatment.  Blog.
		 */
		vga->crt[0x66] = 0x89;
		vga->crt[0x67] = 0;
		vga->crt[0x85] = 0x02;
		vga->crt[0x31] |= 0x08;

		vga->crt[0x13] = (width>>3) & 0xFF;
		vga->crt[0x51] &= ~0x30;
		vga->crt[0x51] |= (width>>7) & 0x30;

		/* pull screen width from GBD for graphics engine. */
		vga->crt[0x50] = 0xC1;	/* set color mode */

		/*
		 * The high nibble is the mode; or'ing in 0x02 turns
		 * on support for gamma correction via the DACs, but I
		 * haven't figured out how to turn on the 8-bit DACs,
		 * so gamma correction stays off.
		 */
		switch(vga->mode->z){
		default:
			error("%d-bit mode not supported on savage 4\n", vga->mode->z);
		case 8:
			vga->crt[0x67] |= 0x00;
			vga->crt[0x50] |= 0<<4;
			break;
		case 15:
			vga->crt[0x67] |= 0x20;
			vga->crt[0x50] |= 1<<4;
			break;
		case 16:
			vga->crt[0x67] |= 0x40;
			vga->crt[0x50] |= 1<<4;
			/*
			 * Manual says this should be accompanied by setting
			 * the clock-doubled modes but this seems to be the
			 * right answer.
			 * Should check if using doubled modes tidies any of
			 * this up.
			 */
			if(id == 0x8C12 || id == 0x8C2E || id == 0x8C10)
				vga->crt[0x67] |= 0x10;
			break;
		case 32:
			vga->crt[0x67] |= 0xD0;
			vga->crt[0x50] |= 3<<4;
			break;
		}
		break;
	}

	/*
	 * Clock bits. If the desired video clock is
	 * one of the two standard VGA clocks it can just be
	 * set using bits <3:2> of vga->misc, otherwise we
	 * need to programme the DCLK PLL.
	 */
	if(val = dbattr(vga->mode->attr, "noclockset")){
		 if((noclockset = strtol(val, &p, 0)) == 0 && p == val)
			error("%s: invalid 'noclockset' attr\n", ctlr->name);
	}
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	vga->misc &= ~0x0C;
	if(vga->f[0] == VgaFreq0){
		/* nothing to do */;
	}
	else if(vga->f[0] == VgaFreq1)
		vga->misc |= 0x04;
	else if(noclockset){
		/*
		 * Don't touch the clock on the Aurora64V+
		 * and optionally on some others.
		 */
		vga->misc |= 0x0C;
	}
	else{
		if(vga->f[0] > pclk)
			error("%s: invalid pclk - %lud\n",
				ctlr->name, vga->f[0]);

		trio64clock(vga, ctlr);
		switch(id){

		case 0x8A10:			/* ViRGE/GX2 */
			vga->sequencer[0x12] = (vga->r[0]<<6)|vga->n[0];
			if(vga->r[0] & 0x04)
				vga->sequencer[0x29] |= 0x01;
			else
				vga->sequencer[0x29] &= ~0x01;
			break;

		case 0x8C2E:			/* SuperSavage/IXC16 (let's try this -rsc) */
		case 0x8C10:			/* Savage MX/MV */
		case 0x8C12:			/* Savage4/IX-MV */
		case 0x8A22:			/* Savage4 */
		case 0x8A25:			/* ProSavage PN133 */
		case 0x8A26:			/* ProSavage KN133 */
		case 0x8D04:			/* ProSavage DDR, K.Okamoto */
			vga->sequencer[0x12] = (vga->r[0]<<6)|(vga->n[0] & 0x3F);
			vga->sequencer[0x39] &= ~0x01;
			vga->sequencer[0x29] &= ~0x1C;
			if(vga->r[0] & 0x04)
				vga->sequencer[0x29] |= (1<<2);
			if(vga->m[0] & 0x100)
				vga->sequencer[0x29] |= (1<<3);
			if(vga->n[0] & 0x40)
				vga->sequencer[0x29] |= (1<<4);
			break;

		default:
			vga->sequencer[0x12] = (vga->r[0]<<5)|vga->n[0];
			break;
		}
		vga->sequencer[0x13] = vga->m[0];
		vga->misc |= 0x0C;
	}

	/*
	 * Internal clock generator.
	 */
	vga->sequencer[0x15] &= ~0x31;
	vga->sequencer[0x15] |= 0x02;
	vga->sequencer[0x18] &= ~0x80;

	/*
	 * Start display FIFO fetch.
	 */
	x = (vga->crt[0]+vga->crt[4]+1)/2;
	vga->crt[0x3B] = x;
	if(x & 0x100)
		vga->crt[0x5D] |= 0x40;

	/*
	 * Display memory access control.
	 * Calculation of the M-parameter (Crt54) is
	 * memory-system and dot-clock dependent, the
	 * values below are guesses from dumping
	 * registers.
	 * The Savage4 does not document 0x54,
	 * but we leave this anyway.
	 */
	if(vga->mode->x <= 800)
		vga->crt[0x54] = 0xE8;
	else if(vga->mode->x <= 1024 && id != 0x8C12 && id != 0x8C2E)
		vga->crt[0x54] = 0xA8;
	else
		vga->crt[0x54] = 0x00/*0x48*/;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int id;
	ushort advfunc;

	s3generic.load(vga, ctlr);

	/*
	 * Load the PLL registers if necessary.
	 * Not sure if the variable-delay method of setting the
	 * PLL will work without a write here to vga->misc,
	 * so use the immediate-load method by toggling bit 5
	 * of Seq15 if necessary.
	 */
	vgaxo(Seqx, 0x12, vga->sequencer[0x12]);
	vgaxo(Seqx, 0x13, vga->sequencer[0x13]);
	id = (vga->crt[0x2D]<<8)|vga->crt[0x2E];
	switch(id){
	case 0x883D:				/* ViRGE/VX*/
		vgaxo(Crtx, 0x36, vga->crt[0x36]);
		break;
	case 0x8A10:				/* ViRGE/GX2 */
		vgaxo(Seqx, 0x29, vga->sequencer[0x29]);
		break;
	case 0x8C2E:				/* SuperSavage/IXC16 (let's try this -rsc) */
	case 0x8C12:				/* Savage4/IX-MV */
		vgaxo(Crtx, 0x90, vga->crt[0x90]);
		vgaxo(Crtx, 0x91, vga->crt[0x91]);
		/*FALLTHROUGH*/
	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
	case 0x8D04:				/* ProSavage DDR, K.Okamoto */
		vgaxo(Seqx, 0x29, vga->sequencer[0x29]);
		vgaxo(Seqx, 0x39, vga->sequencer[0x39]);
		break;
	}
	if((vga->misc & 0x0C) == 0x0C)
		vgaxo(Seqx, 0x15, vga->sequencer[0x15]|0x20);
	vgaxo(Seqx, 0x15, vga->sequencer[0x15]);
	vgaxo(Seqx, 0x18, vga->sequencer[0x18]);

	vgaxo(Crtx, 0x60, vga->crt[0x60]);
	vgaxo(Crtx, 0x63, vga->crt[0x63]);
	vgaxo(Crtx, 0x65, vga->crt[0x65]);
	vgaxo(Crtx, 0x66, vga->crt[0x66]);
	vgaxo(Crtx, 0x67, vga->crt[0x67]);

	switch(id){

	case 0x8810:				/* Microsoft Virtual PC 2004 */
	case 0x8811:				/* Trio64+ */
		advfunc = 0x0000;
		if(ctlr->flag & Uenhanced)
			advfunc = 0x0001;
		outportw(0x4AE8, advfunc);
		break;
	case 0x8901:				/* Trio64V2 */
	case 0x8A01:				/* ViRGE/[DG]X */
		vgaxo(Crtx, 0x90, vga->crt[0x90]);
		vgaxo(Crtx, 0x91, vga->crt[0x91]);
		break;
	case 0x8A10:				/* ViRGE/GX2 */
		vgaxo(Crtx, 0x90, vga->crt[0x90]);
		vgaxo(Crtx, 0x31, vga->crt[0x31]);
		vgaxo(Crtx, 0x13, vga->crt[0x13]);
		vgaxo(Crtx, 0x51, vga->crt[0x51]);
		vgaxo(Crtx, 0x85, vga->crt[0x85]);
		break;
	case 0x8D04:				/* ProSavage DDR, K.Okamoto */
		vgaxo(Crtx, 0x90, vga->crt[0x90]);		//K.Okamoto
		vgaxo(Crtx, 0x91, vga->crt[0x91]);		//K.Okamoto
	case 0x8C2E:				/* SuperSavage/IXC16 (let's try this -rsc) */
	case 0x8C12:				/* Savage4/IX-MV */
	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
		vgaxo(Crtx, 0x31, vga->crt[0x31]);
		vgaxo(Crtx, 0x13, vga->crt[0x13]);
		vgaxo(Crtx, 0x51, vga->crt[0x51]);
		vgaxo(Crtx, 0x85, vga->crt[0x85]);
		vgaxo(Crtx, 0x50, vga->crt[0x50]);
		break;
	}
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i, id;
	ulong dclk, m, n, r;

	s3generic.dump(vga, ctlr);
	printitem(ctlr->name, "Crt70");
	for(i = 0x70; i < 0x99; i++)
		printreg(vga->crt[i]);

	printitem(ctlr->name, "Seq08");
	for(i = 0x08; i < 0x10; i++)
		printreg(vga->sequencer[i]);
	printitem(ctlr->name, "Seq10");
	for(i = 0x10; i < 0x50; i++)
		printreg(vga->sequencer[i]);
	id = (vga->crt[0x2D]<<8)|vga->crt[0x2E];
	switch(id){
	default:
		break;

	case 0x8812:				/* Aurora64V+ */
	case 0x8C2E:				/* SuperSavage/IXC16 (let's try this -rsc) */
	case 0x8C12:				/* Savage4/IX-MV */
		printitem(ctlr->name, "Seq50");
		for(i = 0x50; i < 0x70; i++)
			printreg(vga->sequencer[i]);
		break;
	}

	printitem(ctlr->name, "Crt2D");
	printreg(vga->crt[0x2D]);
	printreg(vga->crt[0x2E]);
	printreg(vga->crt[0x2F]);

	m = vga->sequencer[0x13] & vga->m[1];
	n = vga->sequencer[0x12] & vga->n[1];
	r = (vga->sequencer[0x12]>>5) & 0x03;

	switch(id){
	case 0x8812:				/* Aurora64V+ */
		r = (vga->sequencer[0x12]>>6) & 0x03;
		break;
	case 0x8A01:				/* ViRGE/[DG]X */
		r = (vga->sequencer[0x12]>>5) & 0x07;
		break;
	case 0x8A10:				/* ViRGE/GX2 */
		r = (vga->sequencer[0x12]>>6) & 0x03;
		r |= (vga->sequencer[0x29] & 0x01)<<2;
		break;
	case 0x8C2E:				/* SuperSavage/IXC16 (let's try this -rsc) */
	case 0x8C12:				/* Savage4/IX-MV */
	case 0x8A22:				/* Savage4 */
	case 0x8A25:				/* ProSavage PN133 */
	case 0x8A26:				/* ProSavage KN133 */
	case 0x8D04:				/* Prosavage DDR, K.Okamoto */
		m = vga->sequencer[0x13] & 0xFF;
		if(vga->sequencer[0x29] & (1<<3))
			m |= 0x100;
		if(vga->sequencer[0x29] & (1<<4))
			n |= 0x40;
		r = (vga->sequencer[0x12]>>6) & 0x03;
		r |= (vga->sequencer[0x29] & (1<<2));
		break;
	}

	dclk = (m+2)*RefFreq;
	dclk /= (n+2)*(1<<r);
	printitem(ctlr->name, "dclk m n r");
	Bprint(&stdout, "%9ld %8ld       - %8ld %8ld\n", dclk, m, n, r);

	m = vga->sequencer[0x11] & 0x7F;
	n = vga->sequencer[0x10] & 0x1F;
	r = (vga->sequencer[0x10]>>5) & 0x03;	/* might be GX/2 specific */
	dclk = (m+2)*RefFreq;
	dclk /= (n+2)*(1<<r);
	printitem(ctlr->name, "mclk m n r");
	Bprint(&stdout, "%9ld %8ld       - %8ld %8ld\n", dclk, m, n, r);
}

Ctlr virge = {
	"virge",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
