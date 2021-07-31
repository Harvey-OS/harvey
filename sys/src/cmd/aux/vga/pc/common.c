/*
 * This file is common to both the Plan 9 'vga' program and the DOS 'dm'
 * program.
 */


char *vganames[] = {
	"generic",
	"tseng",
	"pvga1a",
	"ati",
	"trident",
	0,
};

void
arout(ushort reg, uchar val) {
	inb(0x3DA);	/* reset flip-flop */
	if (reg <= 0xf) {
		outb(ARW, reg | 0x0);
		outb(ARW, val);
		inb(0x3DA);
		outb(ARW, reg | 0x20);
	} else {
		outb(ARW, reg | 0x20);
		outb(ARW, val);
	}
}

void
crout(ushort reg, uchar val)
{
	outb(CRX, reg);
	outb(CR, val);
}

void
grout(ushort reg, uchar val)
{
	outb(GRX, reg);
	outb(GR, val);
}

void
srout(ushort reg, uchar val)
{
	outb(SRX, reg);
	outb(SR, val);
}

uchar
arin(ushort i) {
	inb(0x3DA);
	outb(ARW, i | 0x20);
	return inb(ARR);
}

uchar
crin(ushort i) {
	outb(CRX, i);
	return inb(CR);
}

uchar
grin(ushort i) {
	outb(GRX, i);
	return inb(GR);
}

uchar
srin(ushort i) {
	outb(SRX, i);
	return inb(SR);
}

/*
 * ATI extended registers.
 */ 
uchar
extin(ushort i) {
	outb(extport, i);
	return inb(extport+1);
}

void
extout(ushort i, uchar val) {
	outb(extport, i);
	outb(extport, val);
}

uchar
getclock(enum vgatype t) {
	int c = ((grin(0) & 0x0c)>>2);

	switch (t) {
	case tseng:
		c |= ((crin(0x34)&0x02)<<1);
	}
	return c;
}

/*
 * Get the registers needed for the current display mode.
 * The vga type is needed (this routine does not try to
 * figure this out.)  If type is ati, the extport is needed
 * and must be fetched beforehand from the ROM.
 */

void
getmode(VGAmode *v, enum vgatype t) {
	int i;
	v->type = t;

	v->general[0] = inb(EMISCR);	/* misc output */
	v->general[1] = inb(EFCR);	/* feature control */
	for(i = 0; i < sizeof(v->sequencer); i++)
		v->sequencer[i] = srin(i);
	for(i = 0; i < sizeof(v->crt); i++) 
		v->crt[i] = crin(i);
	for(i = 0; i < sizeof(v->graphics); i++) 
		v->graphics[i] = grin(i);
	for(i = 0; i < sizeof(v->attribute); i++)
		v->attribute[i] = arin(i);

	switch (v->type) {
	case generic:
		break;
	case pvga1a:
		v->specials.pvga1a.gr9 = grin(9);
		v->specials.pvga1a.gra = grin(0xa);
		v->specials.pvga1a.grb = grin(0xb);
		v->specials.pvga1a.grc = grin(0xc);
		v->specials.pvga1a.grd = grin(0xd);
		v->specials.pvga1a.gre = grin(0xe);
		v->specials.pvga1a.grf = grin(0xf);
		break;
	case tseng:
		v->specials.tseng.viden = inb(0x3c3);
		v->specials.tseng.sr6  = srin(6);
		v->specials.tseng.sr7  = srin(7);
		v->specials.tseng.ar16 = arin(0x16);
		v->specials.tseng.ar17 = arin(0x17);
		v->specials.tseng.crt31= crin(0x31);
		v->specials.tseng.crt32= crin(0x32);
		v->specials.tseng.crt33= crin(0x33);
		v->specials.tseng.crt34= crin(0x34);
		v->specials.tseng.crt35= crin(0x35);
		v->specials.tseng.crt36= crin(0x36);
		v->specials.tseng.crt37= crin(0x37);
		break;
	case ati:
		v->specials.ati.extport = extport;
		for (i=0xa0; i<=0xbf; i++)
			v->specials.ati.ext[i-0xa0] = extin(i);
		for (i=0; i<8; i++) {
			v->specials.ati.four6e[i] = inb(0x46e8 + i);
			v->specials.ati.four6f[i] = inb(0x46f8 + i);
		}
		break;
	case trident:
		for (i=0; i<sizeof(v->specials.trident); i++)
			v->specials.trident.ext[i] = srin(i+sizeof(v->sequencer));
		break;
	}
}

void
writeregisters(FILE *fd, VGAmode *v) {
	int i;

	fprintf(fd, "\t/* general */\n\t");
	for (i=0; i<sizeof(v->general); i++)
		fprintf(fd, "0x%.2x, ", v->general[i]);
	fprintf(fd, "\n\t/* sequence */\n\t");
	for (i=0; i<sizeof(v->sequencer); i++) {
		if (i>0 && i%8 == 0)
			fprintf(fd, "\n\t");
		fprintf(fd, "0x%.2x, ", v->sequencer[i]);
	}
	fprintf(fd, "\n\t/* crt */\n\t");
	for (i=0; i<sizeof(v->crt); i++) {
		if (i>0 && i%8 == 0)
			fprintf(fd, "\n\t");
		fprintf(fd, "0x%.2x, ", v->crt[i]);
	}
	fprintf(fd, "\n\t/* graphics */\n\t");
	for (i=0; i<sizeof(v->graphics); i++) {
		if (i>0 && i%8 == 0)
			fprintf(fd, "\n\t");
		fprintf(fd, "0x%.2x, ", v->graphics[i]);
	}
	fprintf(fd, "\n\t/* attribute */\n\t");
	for (i=0; i<sizeof(v->attribute); i++) {
		if (i>0 && i%8 == 0)
			fprintf(fd, "\n\t");
		fprintf(fd, "0x%.2x, ", v->attribute[i]);
	}
	fprintf(fd, "\n");
	fprintf(fd, "\t/* special */\n");

	fprintf(fd, "\t\"%s\",", vganames[v->type]);

	switch (v->type) {
	case generic:
		fprintf(fd, "\n");
		break;
	case tseng:
		for (i=0; i<sizeof(v->specials.tseng); i++) {
			if (i%8 == 0)
				fprintf(fd, "\n\t");
			fprintf(fd, "0x%.2x, ", v->specials.generic.dummy[i]);
		}
		break;
	case pvga1a:
		for (i=0; i<sizeof(v->specials.pvga1a); i++) {
			if (i%8 == 0)
				fprintf(fd, "\n\t");
			fprintf(fd, "0x%.2x, ", v->specials.generic.dummy[i]);
		}
		break;
	case ati:
		for (i=0; i<sizeof(v->specials.ati); i++) {
			if (i%8 == 0)
				fprintf(fd, "\n\t");
			fprintf(fd, "0x%.2x, ", v->specials.generic.dummy[i]);
		}
		break;
	}
	fprintf(fd, "\n");
};

void
doreg(uchar in(ushort), void out(ushort, uchar), char *cp) {
	ushort port;
	uchar val;
	int n;
	
	n = sscanf(cp, "%x=%x", &port, &val);
	if (n == 2) {
		out(port, val);
	} else
		fprintf(stderr, "0x%.2x\n", in(port));
	return;
}
