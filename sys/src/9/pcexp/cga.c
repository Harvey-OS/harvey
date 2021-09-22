/*
 * CGA
 */

enum {
	Black,
	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Brown,
	Grey,

	Bright = 0x08,
	Yellow = Bright|Brown,
	White  = Bright|Grey,

	Cgaidxport = 0x3d4,		/* access cga registers */
	Cgadataport,
};

enum {
	Width = 80*2,
	Height = 25,
	Attr = (Black<<4)|Grey,
};

#define CGA ((uchar*)(CGAMEM-KZERO))  /* physical addr. of cga display memory */

static unsigned cgapos;

static uchar
cgaregr(uchar index)
{
	outb(Cgaidxport, index);
	return inb(Cgadataport) & 0xFF;
}

static void
cgaregw(uchar index, uchar data)
{
	outb(Cgaidxport, index);
	outb(Cgadataport, data);
}

static void
movecursor(void)
{
	cgaregw(0x0E, (cgapos/2>>8) & 0xFF);
	cgaregw(0x0F, cgapos/2 & 0xFF);
	CGA[cgapos+1] = Attr;
}

void
cgaputc(int c)
{
	int i;
	uchar *p;

	if(c == '\n'){
		cgapos /= Width;
		cgapos = (cgapos+1)*Width;
	}
	else if(c == '\t'){
		i = 8 - ((cgapos/2)&7);
		while(i-->0)
			cgaputc(' ');
	}
	else if(c == '\b'){
		if(cgapos >= 2)
			cgapos -= 2;
		cgaputc(' ');
		cgapos -= 2;
	}
	else{
		CGA[cgapos++] = c;
		CGA[cgapos++] = Attr;
	}
	if(cgapos >= Width*Height){
		memmove(CGA, &CGA[Width], Width*(Height-1));
		p = &CGA[Width*(Height-1)];
		for(i=0; i<Width/2; i++){
			*p++ = ' ';
			*p++ = Attr;
		}
		cgapos = Width*(Height-1);
	}
	movecursor();
}

void
cgainit(void)
{
	cgapos = cgaregr(0x0E)<<8;
	cgapos |= cgaregr(0x0F);
	cgapos *= 2;
}
