extern uchar debug[];

enum {
	DXFlush	= 'x',		/* special markers for debugging on the commands, disrupts behaviour*/

	DFile		= 'f',		/* Reads, writes, flushes*/
	DPath	= 'p',		/* path for state transitions for tap debugging */
	DState	= 's',		/* state calculation and changes on tap interface */
	Dinst	= 'i',		/* mpsse instruction assembling debug */
	Dassln	= 'a',		/* print instructions before assembling */
	Dmach	= 'm',	/* print mpsse machine code and op results */
	Djtag	= 'j',		/* print jtag level operations */
	Dice		= 'e',		/* print icert level operations */
	Dchain	= 'h',		/* print icert chains */
	Dmmu	= 'u',		/* print MMU ops */
	Dctxt	= 'c',		/* dump context in and out */
	Darm	= 'w',		/* arm insts and so */
	Dmem	= 'y',		/* memory ops */
	Dfs		= 'k',		/* filesystem ops */
	DAll		= 'A'
};

extern int	dprint(char d, char *fmt, ...);
extern void	dumpbuf(char d, uchar *buf, int bufsz);
