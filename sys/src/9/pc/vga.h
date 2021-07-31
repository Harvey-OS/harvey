/*
 * Generic VGA registers.
 */
enum {
	MiscW		= 0x03C2,	/* Miscellaneous Output (W) */
	MiscR		= 0x03CC,	/* Miscellaneous Output (R) */
	Status0		= 0x03C2,	/* Input status 0 (R) */
	Status1		= 0x03DA,	/* Input Status 1 (R) */
	FeatureR	= 0x03CA,	/* Feature Control (R) */
	FeatureW	= 0x03DA,	/* Feature Control (W) */

	Seqx		= 0x03C4,	/* Sequencer Index, Data at Seqx+1 */
	Crtx		= 0x03D4,	/* CRT Controller Index, Data at Crtx+1 */
	Grx		= 0x03CE,	/* Graphics Controller Index, Data at Grx+1 */
	Attrx		= 0x03C0,	/* Attribute Controller Index and Data */

	PaddrW		= 0x03C8,	/* Palette Address Register, write */
	Pdata		= 0x03C9,	/* Palette Data Register */
	Pixmask		= 0x03C6,	/* Pixel Mask Register */
	PaddrR		= 0x03C7,	/* Palette Address Register, read */
	Pstatus		= 0x03C7,	/* DAC Status (RO) */

	Pcolours	= 256,		/* Palette */
	Pred		= 0,
	Pgreen		= 1,
	Pblue		= 2,
};

#define vgai(port)		inb(port)
#define vgao(port, data)	outb(port, data)

extern int vgaxi(long, uchar);
extern int vgaxo(long, uchar, uchar);

/*
 * Definitions of known VGA controllers.
 */
typedef struct Vgac Vgac;
struct Vgac {
	char	*name;
	void	(*page)(int);

	Vgac*	link;
};

/*
 * Definition of known hardware graphics cursors.
 */
typedef struct Hwgc Hwgc;
struct Hwgc {
	char*	name;
	void	(*enable)(void);
	void	(*load)(Cursor*);
	int	(*move)(Point);
	void	(*disable)(void);

	Hwgc*	link;
};

extern void addvgaclink(Vgac*);
extern void addhwgclink(Hwgc*);

extern Hwgc *hwgc;

extern Lock palettelock;
