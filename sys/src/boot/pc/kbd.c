#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

enum {
	Data=		0x60,	/* data port */

	Status=		0x64,	/* status port */
	 Inready=	0x01,	/*  input character ready */
	 Outbusy=	0x02,	/*  output busy */
	 Sysflag=	0x04,	/*  system flag */
	 Cmddata=	0x08,	/*  cmd==0, data==1 */
	 Inhibit=	0x10,	/*  keyboard/mouse inhibited */
	 Minready=	0x20,	/*  mouse character ready */
	 Rtimeout=	0x40,	/*  general timeout */
	 Parity=	0x80,

	Cmd=		0x64,	/* command port (write only) */
 
	Spec=	0x80,

	PF=	Spec|0x20,	/* num pad function key */
	View=	Spec|0x00,	/* view (shift window up) */
	KF=	Spec|0x40,	/* function key */
	Shift=	Spec|0x60,
	Break=	Spec|0x61,
	Ctrl=	Spec|0x62,
	Latin=	Spec|0x63,
	Caps=	Spec|0x64,
	Num=	Spec|0x65,
	No=	Spec|0x7F,	/* no mapping */

	Home=	KF|13,
	Up=	KF|14,
	Pgup=	KF|15,
	Print=	KF|16,
	Left=	View,
	Right=	View,
	End=	'\r',
	Down=	View,
	Pgdown=	View,
	Ins=	KF|20,
	Del=	0x7F,
};

uchar kbtab[] = 
{
[0x00]	No,	0x1b,	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	No,
[0x38]	Latin,	' ',	Caps,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	Home,
[0x48]	No,	No,	No,	No,	No,	No,	No,	No,
[0x50]	No,	No,	No,	No,	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

uchar kbtabshift[] =
{
[0x00]	No,	0x1b,	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	No,
[0x38]	Latin,	' ',	Caps,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	Home,
[0x48]	No,	No,	No,	No,	No,	No,	No,	No,
[0x50]	No,	No,	No,	No,	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

uchar kbtabesc1[] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	No,	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	No,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	No,	No,	Print,
[0x38]	Latin,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Down,	No,	Right,	No,	End,
[0x50]	Left,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
};

struct latin
{
	uchar	l;
	char	c[2];
}latintab[] = {
	L'¡',	"!!",	/* spanish initial ! */
	L'¢',	"c|",	/* cent */
	L'¢',	"c$",	/* cent */
	L'£',	"l$",	/* pound sterling */
	L'¤',	"g$",	/* general currency */
	L'¥',	"y$",	/* yen */
	L'¥',	"j$",	/* yen */
	L'¦',	"||",	/* broken vertical bar */
	L'§',	"SS",	/* section symbol */
	L'¨',	"\"\"",	/* dieresis */
	L'©',	"cr",	/* copyright */
	L'©',	"cO",	/* copyright */
	L'ª',	"sa",	/* super a, feminine ordinal */
	L'«',	"<<",	/* left angle quotation */
	L'¬',	"no",	/* not sign, hooked overbar */
	L'­',	"--",	/* soft hyphen */
	L'®',	"rg",	/* registered trademark */
	L'¯',	"__",	/* macron */
	L'°',	"s0",	/* degree (sup o) */
	L'±',	"+-",	/* plus-minus */
	L'²',	"s2",	/* sup 2 */
	L'³',	"s3",	/* sup 3 */
	L'´',	"''",	/* grave accent */
	L'µ',	"mu",	/* mu */
	L'¶',	"pg",	/* paragraph (pilcrow) */
	L'·',	"..",	/* centered . */
	L'¸',	",,",	/* cedilla */
	L'¹',	"s1",	/* sup 1 */
	L'º',	"so",	/* sup o */
	L'»',	">>",	/* right angle quotation */
	L'¼',	"14",	/* 1/4 */
	L'½',	"12",	/* 1/2 */
	L'¾',	"34",	/* 3/4 */
	L'¿',	"??",	/* spanish initial ? */
	L'À',	"A`",	/* A grave */
	L'Á',	"A'",	/* A acute */
	L'Â',	"A^",	/* A circumflex */
	L'Ã',	"A~",	/* A tilde */
	L'Ä',	"A\"",	/* A dieresis */
	L'Ä',	"A:",	/* A dieresis */
	L'Å',	"Ao",	/* A circle */
	L'Å',	"AO",	/* A circle */
	L'Æ',	"Ae",	/* AE ligature */
	L'Æ',	"AE",	/* AE ligature */
	L'Ç',	"C,",	/* C cedilla */
	L'È',	"E`",	/* E grave */
	L'É',	"E'",	/* E acute */
	L'Ê',	"E^",	/* E circumflex */
	L'Ë',	"E\"",	/* E dieresis */
	L'Ë',	"E:",	/* E dieresis */
	L'Ì',	"I`",	/* I grave */
	L'Í',	"I'",	/* I acute */
	L'Î',	"I^",	/* I circumflex */
	L'Ï',	"I\"",	/* I dieresis */
	L'Ï',	"I:",	/* I dieresis */
	L'Ð',	"D-",	/* Eth */
	L'Ñ',	"N~",	/* N tilde */
	L'Ò',	"O`",	/* O grave */
	L'Ó',	"O'",	/* O acute */
	L'Ô',	"O^",	/* O circumflex */
	L'Õ',	"O~",	/* O tilde */
	L'Ö',	"O\"",	/* O dieresis */
	L'Ö',	"O:",	/* O dieresis */
	L'Ö',	"OE",	/* O dieresis */
	L'Ö',	"Oe",	/* O dieresis */
	L'×',	"xx",	/* times sign */
	L'Ø',	"O/",	/* O slash */
	L'Ù',	"U`",	/* U grave */
	L'Ú',	"U'",	/* U acute */
	L'Û',	"U^",	/* U circumflex */
	L'Ü',	"U\"",	/* U dieresis */
	L'Ü',	"U:",	/* U dieresis */
	L'Ü',	"UE",	/* U dieresis */
	L'Ü',	"Ue",	/* U dieresis */
	L'Ý',	"Y'",	/* Y acute */
	L'Þ',	"P|",	/* Thorn */
	L'Þ',	"Th",	/* Thorn */
	L'Þ',	"TH",	/* Thorn */
	L'ß',	"ss",	/* sharp s */
	L'à',	"a`",	/* a grave */
	L'á',	"a'",	/* a acute */
	L'â',	"a^",	/* a circumflex */
	L'ã',	"a~",	/* a tilde */
	L'ä',	"a\"",	/* a dieresis */
	L'ä',	"a:",	/* a dieresis */
	L'å',	"ao",	/* a circle */
	L'æ',	"ae",	/* ae ligature */
	L'ç',	"c,",	/* c cedilla */
	L'è',	"e`",	/* e grave */
	L'é',	"e'",	/* e acute */
	L'ê',	"e^",	/* e circumflex */
	L'ë',	"e\"",	/* e dieresis */
	L'ë',	"e:",	/* e dieresis */
	L'ì',	"i`",	/* i grave */
	L'í',	"i'",	/* i acute */
	L'î',	"i^",	/* i circumflex */
	L'ï',	"i\"",	/* i dieresis */
	L'ï',	"i:",	/* i dieresis */
	L'ð',	"d-",	/* eth */
	L'ñ',	"n~",	/* n tilde */
	L'ò',	"o`",	/* o grave */
	L'ó',	"o'",	/* o acute */
	L'ô',	"o^",	/* o circumflex */
	L'õ',	"o~",	/* o tilde */
	L'ö',	"o\"",	/* o dieresis */
	L'ö',	"o:",	/* o dieresis */
	L'ö',	"oe",	/* o dieresis */
	L'÷',	"-:",	/* divide sign */
	L'ø',	"o/",	/* o slash */
	L'ù',	"u`",	/* u grave */
	L'ú',	"u'",	/* u acute */
	L'û',	"u^",	/* u circumflex */
	L'ü',	"u\"",	/* u dieresis */
	L'ü',	"u:",	/* u dieresis */
	L'ü',	"ue",	/* u dieresis */
	L'ý',	"y'",	/* y acute */
	L'þ',	"th",	/* thorn */
	L'þ',	"p|",	/* thorn */
	L'ÿ',	"y\"",	/* y dieresis */
	L'ÿ',	"y:",	/* y dieresis */
	0,	0,
};

enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cmousedis=	(1<<5),		/* mouse disable */
	Ckbddis=	(1<<4),		/* kbd disable */
	Csf=		(1<<2),		/* system flag */
	Cmouseint=	(1<<1),		/* mouse interrupt enable */
	Ckbdint=	(1<<0),		/* kbd interrupt enable */
};

static uchar ccc;

int
latin1(int k1, int k2)
{
	struct latin *l;

	for(l=latintab; l->l; l++)
		if(k1==l->c[0] && k2==l->c[1])
			return l->l;
	return 0;
}

/*
 *  wait for output no longer busy
 */
static int
outready(void)
{
	int tries;

	for(tries = 0; (inb(Status) & Outbusy); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

/*
 *  wait for input
 */
static int
inready(void)
{
	int tries;

	for(tries = 0; !(inb(Status) & Inready); tries++){
		if(tries > 500)
			return -1;
		delay(2);
	}
	return 0;
}

/*
 *  ask 8042 to enable the use of address bit 20
 */
void
i8042a20(void)
{
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDF);
	outready();
}

/*
 *  ask 8042 to reset the machine
 */
void
i8042reset(void)
{
	int i, x;
#ifdef notdef
	ushort *s = (ushort*)(KZERO|0x472);

	*s = 0x1234;		/* BIOS warm-boot flag */
#endif /* notdef */

	outready();
	outb(Cmd, 0xFE);	/* pulse reset line (means resend on AT&T machines) */
	outready();

	/*
	 *  Pulse it by hand (old somewhat reliable)
	 */
	x = 0xDF;
	for(i = 0; i < 5; i++){
		x ^= 1;
		outready();
		outb(Cmd, 0xD1);
		outready();
		outb(Data, x);	/* toggle reset */
		delay(100);
	}
}

/*
 *  keyboard interrupt
 */
static void
i8042intr(Ureg*, void*)
{
	int s, c;
	static int esc1, esc2;
	static int alt, caps, ctl, num, shift;
	static int lstate, k1, k2;
	int keyup;

	/*
	 *  get status
	 */
	s = inb(Status);
	if(!(s&Inready))
		return;

	/*
	 *  get the character
	 */
	c = inb(Data);

	/*
	 *  if it's the aux port...
	 */
	if(s & Minready)
		return;

	/*
	 *  e0's is the first of a 2 character sequence
	 */
	if(c == 0xe0){
		esc1 = 1;
		return;
	} else if(c == 0xe1){
		esc2 = 2;
		return;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		c |= keyup;
		if(c != 0xFF)	/* these come fairly often: CAPSLOCK U Y */
			print("unknown key %ux\n", c);
		return;
	}

	if(esc1){
		c = kbtabesc1[c];
		esc1 = 0;
	} else if(esc2){
		esc2--;
		return;
	} else if(shift)
		c = kbtabshift[c];
	else
		c = kbtab[c];

	if(caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Latin:
			alt = 0;
			break;
		case Shift:
			shift = 0;
			break;
		case Ctrl:
			ctl = 0;
			break;
		}
		return;
	}

	/*
 	 *  normal character
	 */
	if(!(c & Spec)){
		if(ctl){
			if(alt && c == Del)
				warp86("\nCtrl-Alt-Del\n", 0);
			c &= 0x1f;
		}
		switch(lstate){
		case 1:
			k1 = c;
			lstate = 2;
			return;
		case 2:
			k2 = c;
			lstate = 0;
			c = latin1(k1, k2);
			if(c == 0){
				kbdchar(k1);
				c = k2;
			}
			/* fall through */
		default:
			break;
		}
	} else {
		switch(c){
		case Caps:
			caps ^= 1;
			return;
		case Num:
			num ^= 1;
			return;
		case Shift:
			shift = 1;
			return;
		case Latin:
			alt = 1;
			lstate = 1;
			return;
		case Ctrl:
			ctl = 1;
			return;
		}
	}
	kbdchar(c);
}

static char *initfailed = "kbd init failed\n";

void
i8042init(void)
{
	int c;

	/* wait for a quiescent controller */
	while((c = inb(Status)) & (Outbusy | Inready))
		if(c & Inready)
			inb(Data);

	/* get current controller command byte */
	outb(Cmd, 0x20);
	if(inready() < 0){
		print("kbdinit: can't read ccc\n");
		ccc = 0;
	} else
		ccc = inb(Data);

	/* enable kbd xfers and interrupts */
	ccc &= ~Ckbddis;
	ccc |= Csf | Ckbdint | Cscs1;
	if(outready() < 0)
		print(initfailed);
	outb(Cmd, 0x60);
	if(outready() < 0)
		print(initfailed);
	outb(Data, ccc);
	if(outready() < 0)
		print(initfailed);

	setvec(VectorKBD, i8042intr, 0);
}
