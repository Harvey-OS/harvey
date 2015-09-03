/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * ATI Radeon [789]XXX vga driver
 * see /sys/src/cmd/aux/vga/radeon.c
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#define Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

#include "../../cmd/aux/vga/radeon.h"	/* ugh */

/* #define HW_ACCEL */

enum {
	Kilo	= 1024,
	Meg	= Kilo * Kilo,
};

static Pcidev*
radeonpci(void)
{
	static Pcidev *p = nil;
	struct pciids *ids;

	while ((p = pcimatch(p, ATI_PCIVID, 0)) != nil)
		for (ids = radeon_pciids; ids->did; ids++)
			if (ids->did == p->did)
				return p;
	return nil;
}

/* mmio access */

static void
OUTREG8(uint32_t mmio, uint32_t offset, unsigned char val)
{
	((unsigned char*)KADDR((mmio + offset)))[0] = val;
}

static void
OUTREG(uint32_t mmio, uint32_t offset, uint32_t val)
{
	((uint32_t*)KADDR((mmio + offset)))[0] = val;
}

static uint32_t
INREG(uint32_t mmio, uint32_t offset)
{
	return ((uint32_t*)KADDR((mmio + offset)))[0];
}

static void
OUTREGP(uint32_t mmio, uint32_t offset, uint32_t val, uint32_t mask)
{
	OUTREG(mmio, offset, (INREG(mmio, offset) & mask) | val);
}

static void
OUTPLL(uint32_t mmio, uint32_t offset, uint32_t val)
{
	OUTREG8(mmio, CLOCK_CNTL_INDEX, (offset & 0x3f) | PLL_WR_EN);
	OUTREG(mmio, CLOCK_CNTL_DATA, val);
}

static uint32_t
INPLL(uint32_t mmio, uint32_t offset)
{
	OUTREG8(mmio, CLOCK_CNTL_INDEX, offset & 0x3f);
	return INREG(mmio, CLOCK_CNTL_DATA);
}

static void
radeonlinear(VGAscr *scr, int i, int n)
{
}

static void
radeonenable(VGAscr *scr)
{
	Pcidev *p;

	if (scr->mmio)
		return;
	p = radeonpci();
	if (p == nil)
		return;
	scr->id = p->did;
	scr->pci = p;

	scr->mmio = vmap(p->mem[2].bar & ~0x0f, p->mem[2].size);
	if(scr->mmio == 0)
		return;
	addvgaseg("radeonmmio", p->mem[2].bar & ~0x0f, p->mem[2].size);

	vgalinearpci(scr);
	if(scr->apsize)
		addvgaseg("radeonscreen", scr->paddr, scr->apsize);
}

static void
radeoncurload(VGAscr *scr, Cursor *curs)
{
	int x, y;
	uint32_t *p;

	if(scr->mmio == nil)
		return;

	p = (uint32_t*)KADDR(scr->storage);

	for(y = 0; y < 64; y++){
		int cv, sv;

		if (y < 16) {
			cv = curs->clr[2*y] << 8 | curs->clr[2*y+1];
			sv = curs->set[2*y] << 8 | curs->set[2*y+1];
		} else
			cv = sv = 0;

		for(x = 0; x < 64; x++){
			uint32_t col = 0;
			int c, s;

			if (x < 16) {
				c = (cv >> (15 - x)) & 1;
				s = (sv >> (15 - x)) & 1;
			} else
				c = s = 0;

			switch(c | s<<1) {
			case 0:
				col = 0;
				break;
			case 1:
				col = (uint32_t)~0ul;		/* white */
				break;
			case 2:
			case 3:
				col = 0xff000000;	/* black */
				break;
			}

			*p++ = col;
		}
	}

	scr->offset.x = curs->offset.x;
	scr->offset.y = curs->offset.y;
}

static int
radeoncurmove(VGAscr *scr, Point p)
{
	int x, y, ox, oy;
	static uint32_t storage = 0;

	if(scr->mmio == nil)
		return 1;

	if (storage == 0)
		storage = scr->apsize - 1*Meg;

	x = p.x + scr->offset.x;
	y = p.y + scr->offset.y;
	ox = oy = 0;

	if (x < 0) {
		ox = -x - 1;
		x = 0;
	}
	if (y < 0) {
		oy = -y - 1;
		y = 0;
	}

	OUTREG((uint64_t)scr->mmio, CUR_OFFSET, storage + oy * 256);
	OUTREG((uint64_t)scr->mmio, CUR_HORZ_VERT_OFF,
		(ox & 0x7fff) << 16 | (oy & 0x7fff));
	OUTREG((uint64_t)scr->mmio, CUR_HORZ_VERT_POSN,
		(x & 0x7fff) << 16 | (y & 0x7fff));
	return 0;
}

static void
radeoncurdisable(VGAscr *scr)
{
	if(scr->mmio == nil)
		return;
	OUTREGP((uint64_t)scr->mmio, CRTC_GEN_CNTL, 0, ~CRTC_CUR_EN);
}

static void
radeoncurenable(VGAscr *scr)
{
	uint32_t storage;

	if(scr->mmio == 0)
		return;

	radeoncurdisable(scr);
	storage = scr->apsize - 1*Meg;
	scr->storage = (uint64_t)KADDR(scr->paddr + storage);
	radeoncurload(scr, &arrow);
	radeoncurmove(scr, ZP);

	OUTREGP((uint64_t)scr->mmio, CRTC_GEN_CNTL, CRTC_CUR_EN | 2<<20,
		~(CRTC_CUR_EN | 3<<20));
}

/* hw blank */

static void
radeonblank(VGAscr* scr, int blank)
{
	uint32_t mask;
//	char *cp;

	if (scr->mmio == 0)
		return;

//	iprint("radeon: hwblank(%d)\n", blank);

	mask = CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS | CRTC_VSYNC_DIS;
	if (blank == 0) {
		OUTREGP((uint64_t)scr->mmio, CRTC_EXT_CNTL, 0, ~mask);
		return;
	}

	/* One more time this must be kernel param, this is deprecated */
	/* cp = getconf("*dpms");
	if (cp) {
		if (strcmp(cp, "standby") == 0)
			OUTREGP((uint64_t)scr->mmio, CRTC_EXT_CNTL,
				CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS, ~mask);
		else if (strcmp(cp, "suspend") == 0)
			OUTREGP((uint64_t)scr->mmio, CRTC_EXT_CNTL,
				CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS, ~mask);
		else if (strcmp(cp, "off") == 0)
			OUTREGP((uint64_t)scr->mmio, CRTC_EXT_CNTL, mask, ~mask);
		return;
	}
	*/

	OUTREGP((uint64_t)scr->mmio, CRTC_EXT_CNTL, mask, ~mask);
}

/* hw acceleration */

static void
radeonwaitfifo(VGAscr *scr, int entries)
{
	int i;

	for (i = 0; i < 2000000; i++)
		if (INREG((uint64_t)scr->mmio, RBBM_STATUS) & RBBM_FIFOCNT_MASK >=
		    entries)
			return;
	iprint("radeon: fifo timeout\n");
}

static void
radeonwaitidle(VGAscr *scr)
{
	radeonwaitfifo(scr, 64);

	for (; ; ) {
		int i;

		for (i = 0; i < 2000000; i++)
			if (!(INREG((uint64_t)scr->mmio, RBBM_STATUS) & RBBM_ACTIVE))
				return;

		iprint("radeon: idle timed out: %uld entries, stat=0x%.8ulx\n",
			INREG((uint64_t)scr->mmio, RBBM_STATUS) & RBBM_FIFOCNT_MASK,
			INREG((uint64_t)scr->mmio, RBBM_STATUS));
	}
}

static uint32_t dp_gui_master_cntl = 0;

static int
radeonfill(VGAscr *scr, Rectangle r, uint32_t color)
{
	if (scr->mmio == nil)
		return 0;

	radeonwaitfifo(scr, 6);
	OUTREG((uint64_t)scr->mmio, DP_GUI_MASTER_CNTL,
		dp_gui_master_cntl | GMC_BRUSH_SOLID_COLOR |
		GMC_SRC_DATATYPE_COLOR | ROP3_P);
	OUTREG((uint64_t)scr->mmio, DP_BRUSH_FRGD_CLR, color);
	OUTREG((uint64_t)scr->mmio, DP_WRITE_MASK, (uint32_t)~0ul);
	OUTREG((uint64_t)scr->mmio, DP_CNTL,
		DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);
	OUTREG((uint64_t)scr->mmio, DST_Y_X, r.min.y << 16 | r.min.x);
	OUTREG((uint64_t)scr->mmio, DST_WIDTH_HEIGHT, Dx(r) << 16 | Dy(r));

	radeonwaitidle(scr);
	return 1;
}

static int
radeonscroll(VGAscr*scr, Rectangle dst, Rectangle src)
{
	int xs, ys, xd, yd, w, h;
	uint32_t dp_cntl = 0x20;

	if (scr->mmio == nil)
		return 0;

	// iprint("radeon: hwscroll(dst:%R, src:%R)\n", dst, src);

	xd = dst.min.x;
	yd = dst.min.y;
	xs = src.min.x;
	ys = src.min.y;
	w = Dx(dst);
	h = Dy(dst);

	if (ys < yd) {
		ys += h - 1;
		yd += h - 1;
	} else
		dp_cntl |= DST_Y_TOP_TO_BOTTOM;

	if (xs < xd) {
		xs += w - 1;
		xd += w - 1;
	} else
		dp_cntl |= DST_X_LEFT_TO_RIGHT;

	radeonwaitfifo(scr, 6);
	OUTREG((uint64_t)scr->mmio, DP_GUI_MASTER_CNTL, dp_gui_master_cntl |
		GMC_BRUSH_NONE | GMC_SRC_DATATYPE_COLOR | DP_SRC_SOURCE_MEMORY |
		ROP3_S);
	OUTREG((uint64_t)scr->mmio, DP_WRITE_MASK, (uint32_t)~0ul);
	OUTREG((uint64_t)scr->mmio, DP_CNTL, dp_cntl);
	OUTREG((uint64_t)scr->mmio, SRC_Y_X, ys << 16 | xs);
	OUTREG((uint64_t)scr->mmio, DST_Y_X, yd << 16 | xd);
	OUTREG((uint64_t)scr->mmio, DST_WIDTH_HEIGHT, w << 16 | h);

	radeonwaitidle(scr);

	// iprint("radeon: hwscroll(xs=%d ys=%d xd=%d yd=%d w=%d h=%d)\n",
	//	xs, ys, xd, yd, w, h);
	return 1;
}

static void
radeondrawinit(VGAscr*scr)
{
	uint32_t bpp, dtype, i, pitch, clock_cntl_index, mclk_cntl, rbbm_soft_reset;

	if (scr->mmio == 0)
		return;

	switch (scr->gscreen->depth) {
	case 6:
	case 8:
		dtype = 2;
		bpp = 1;
		break;
	case 15:
		dtype = 3;
		bpp = 2;
		break;
	case 16:
		dtype = 4;
		bpp = 2;
		break;
	case 32:
		dtype = 6;
		bpp = 4;
		break;
	default:
		return;
	}

	/* disable 3D */
	OUTREG((uint64_t)scr->mmio, RB3D_CNTL, 0);

	/* flush engine */
	OUTREGP((uint64_t)scr->mmio, RB2D_DSTCACHE_CTLSTAT,
		RB2D_DC_FLUSH_ALL, ~RB2D_DC_FLUSH_ALL);
	for (i = 0; i < 2000000; i++)
		if (!(INREG((uint64_t)scr->mmio, RB2D_DSTCACHE_CTLSTAT) &
		    RB2D_DC_BUSY))
			break;

	/* reset 2D engine */
	clock_cntl_index = INREG((uint64_t)scr->mmio, CLOCK_CNTL_INDEX);

	mclk_cntl = INPLL((uint64_t)scr->mmio, MCLK_CNTL);
	OUTPLL((uint64_t)scr->mmio, MCLK_CNTL, mclk_cntl | FORCEON_MCLKA |
		FORCEON_MCLKB | FORCEON_YCLKA | FORCEON_YCLKB | FORCEON_MC |
		FORCEON_AIC);
	rbbm_soft_reset = INREG((uint64_t)scr->mmio, RBBM_SOFT_RESET);

	OUTREG((uint64_t)scr->mmio, RBBM_SOFT_RESET, rbbm_soft_reset |
		SOFT_RESET_CP | SOFT_RESET_HI | SOFT_RESET_SE | SOFT_RESET_RE |
		SOFT_RESET_PP | SOFT_RESET_E2 | SOFT_RESET_RB);
	INREG((uint64_t)scr->mmio, RBBM_SOFT_RESET);
	OUTREG((uint64_t)scr->mmio, RBBM_SOFT_RESET, rbbm_soft_reset &
		~(SOFT_RESET_CP | SOFT_RESET_HI | SOFT_RESET_SE | SOFT_RESET_RE |
		SOFT_RESET_PP | SOFT_RESET_E2 | SOFT_RESET_RB));
	INREG((uint64_t)scr->mmio, RBBM_SOFT_RESET);

	OUTPLL((uint64_t)scr->mmio, MCLK_CNTL, mclk_cntl);
	OUTREG((uint64_t)scr->mmio, CLOCK_CNTL_INDEX, clock_cntl_index);

	/* init 2D engine */
	radeonwaitfifo(scr, 1);
	OUTREG((uint64_t)scr->mmio, RB2D_DSTCACHE_MODE, 0);

	pitch = Dx(scr->gscreen->r) * bpp;
	radeonwaitfifo(scr, 4);
	OUTREG((uint64_t)scr->mmio, DEFAULT_PITCH, pitch);
	OUTREG((uint64_t)scr->mmio, DST_PITCH, pitch);
	OUTREG((uint64_t)scr->mmio, SRC_PITCH, pitch);
	OUTREG((uint64_t)scr->mmio, DST_PITCH_OFFSET_C, 0);

	radeonwaitfifo(scr, 3);
	OUTREG((uint64_t)scr->mmio, DEFAULT_OFFSET, 0);
	OUTREG((uint64_t)scr->mmio, DST_OFFSET, 0);
	OUTREG((uint64_t)scr->mmio, SRC_OFFSET, 0);

	radeonwaitfifo(scr, 1);
	OUTREGP((uint64_t)scr->mmio, DP_DATATYPE, 0, ~HOST_BIG_ENDIAN_EN);

	radeonwaitfifo(scr, 1);
	OUTREG((uint64_t)scr->mmio, DEFAULT_SC_BOTTOM_RIGHT,
		DEFAULT_SC_RIGHT_MAX | DEFAULT_SC_BOTTOM_MAX);

	dp_gui_master_cntl = dtype << GMC_DST_DATATYPE_SHIFT |
		GMC_SRC_PITCH_OFFSET_CNTL | GMC_DST_PITCH_OFFSET_CNTL |
		GMC_CLR_CMP_CNTL_DIS;
	radeonwaitfifo(scr, 1);
	OUTREG((uint64_t)scr->mmio, DP_GUI_MASTER_CNTL,
	    dp_gui_master_cntl | GMC_BRUSH_SOLID_COLOR | GMC_SRC_DATATYPE_COLOR);

	radeonwaitfifo(scr, 7);
	OUTREG((uint64_t)scr->mmio, DST_LINE_START,    0);
	OUTREG((uint64_t)scr->mmio, DST_LINE_END,      0);
	OUTREG((uint64_t)scr->mmio, DP_BRUSH_FRGD_CLR, (uint32_t)~0ul);
	OUTREG((uint64_t)scr->mmio, DP_BRUSH_BKGD_CLR, 0);
	OUTREG((uint64_t)scr->mmio, DP_SRC_FRGD_CLR,   (uint32_t)~0ul);
	OUTREG((uint64_t)scr->mmio, DP_SRC_BKGD_CLR,   0);
	OUTREG((uint64_t)scr->mmio, DP_WRITE_MASK,     (uint32_t)~0ul);

	radeonwaitidle(scr);

	scr->fill = radeonfill;
	scr->scroll = radeonscroll;
	hwaccel = 1;

	scr->blank = radeonblank;
	hwblank = 1;
}

/* Export */

VGAdev vgaradeondev = {
	"radeon",

	radeonenable,
	0, 				/* disable */
	0, 				/* page */
	radeonlinear,

	radeondrawinit,
#ifdef HW_ACCEL
	radeonfill,

	radeonovlctl,
	radeonovlwrite,
	radeonflush,
#endif
};
VGAcur vgaradeoncur = {
	"radeonhwgc",
	radeoncurenable,
	radeoncurdisable,
	radeoncurload,
	radeoncurmove,
	0				/* doespanning */
};
