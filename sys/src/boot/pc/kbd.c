#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	<libg.h>
#include	<gnot.h>

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
	'¡',	"!!",	/* spanish initial ! */
	'¢',	"c|",	/* cent */
	'¢',	"c$",	/* cent */
	'£',	"l$",	/* pound sterling */
	'¤',	"g$",	/* general currency */
	'¥',	"y$",	/* yen */
	'¥',	"j$",	/* yen */
	'¦',	"||",	/* broken vertical bar */
	'§',	"SS",	/* section symbol */
	'¨',	"\"\"",	/* dieresis */
	'©',	"cr",	/* copyright */
	'©',	"cO",	/* copyright */
	'ª',	"sa",	/* super a, feminine ordinal */
	'«',	"<<",	/* left angle quotation */
	'¬',	"no",	/* not sign, hooked overbar */
	'­',	"--",	/* soft hyphen */
	'®',	"rg",	/* registered trademark */
	'¯',	"__",	/* macron */
	'°',	"s0",	/* degree (sup o) */
	'±',	"+-",	/* plus-minus */
	'²',	"s2",	/* sup 2 */
	'³',	"s3",	/* sup 3 */
	'´',	"''",	/* grave accent */
	'µ',	"mu",	/* mu */
	'¶',	"pg",	/* paragraph (pilcrow) */
	'·',	"..",	/* centered . */
	'¸',	",,",	/* cedilla */
	'¹',	"s1",	/* sup 1 */
	'º',	"so",	/* sup o */
	'»',	">>",	/* right angle quotation */
	'¼',	"14",	/* 1/4 */
	'½',	"12",	/* 1/2 */
	'¾',	"34",	/* 3/4 */
	'¿',	"??",	/* spanish initial ? */
	'À',	"A`",	/* A grave */
	'Á',	"A'",	/* A acute */
	'Â',	"A^",	/* A circumflex */
	'Ã',	"A~",	/* A tilde */
	'Ä',	"A\"",	/* A dieresis */
	'Ä',	"A:",	/* A dieresis */
	'Å',	"Ao",	/* A circle */
	'Å',	"AO",	/* A circle */
	'Æ',	"Ae",	/* AE ligature */
	'Æ',	"AE",	/* AE ligature */
	'Ç',	"C,",	/* C cedilla */
	'È',	"E`",	/* E grave */
	'É',	"E'",	/* E acute */
	'Ê',	"E^",	/* E circumflex */
	'Ë',	"E\"",	/* E dieresis */
	'Ë',	"E:",	/* E dieresis */
	'Ì',	"I`",	/* I grave */
	'Í',	"I'",	/* I acute */
	'Î',	"I^",	/* I circumflex */
	'Ï',	"I\"",	/* I dieresis */
	'Ï',	"I:",	/* I dieresis */
	'Ð',	"D-",	/* Eth */
	'Ñ',	"N~",	/* N tilde */
	'Ò',	"O`",	/* O grave */
	'Ó',	"O'",	/* O acute */
	'Ô',	"O^",	/* O circumflex */
	'Õ',	"O~",	/* O tilde */
	'Ö',	"O\"",	/* O dieresis */
	'Ö',	"O:",	/* O dieresis */
	'Ö',	"OE",	/* O dieresis */
	'Ö',	"Oe",	/* O dieresis */
	'×',	"xx",	/* times sign */
	'Ø',	"O/",	/* O slash */
	'Ù',	"U`",	/* U grave */
	'Ú',	"U'",	/* U acute */
	'Û',	"U^",	/* U circumflex */
	'Ü',	"U\"",	/* U dieresis */
	'Ü',	"U:",	/* U dieresis */
	'Ü',	"UE",	/* U dieresis */
	'Ü',	"Ue",	/* U dieresis */
	'Ý',	"Y'",	/* Y acute */
	'Þ',	"P|",	/* Thorn */
	'Þ',	"Th",	/* Thorn */
	'Þ',	"TH",	/* Thorn */
	'ß',	"ss",	/* sharp s */
	'à',	"a`",	/* a grave */
	'á',	"a'",	/* a acute */
	'â',	"a^",	/* a circumflex */
	'ã',	"a~",	/* a tilde */
	'ä',	"a\"",	/* a dieresis */
	'ä',	"a:",	/* a dieresis */
	'å',	"ao",	/* a circle */
	'æ',	"ae",	/* ae ligature */
	'ç',	"c,",	/* c cedilla */
	'è',	"e`",	/* e grave */
	'é',	"e'",	/* e acute */
	'ê',	"e^",	/* e circumflex */
	'ë',	"e\"",	/* e dieresis */
	'ë',	"e:",	/* e dieresis */
	'ì',	"i`",	/* i grave */
	'í',	"i'",	/* i acute */
	'î',	"i^",	/* i circumflex */
	'ï',	"i\"",	/* i dieresis */
	'ï',	"i:",	/* i dieresis */
	'ð',	"d-",	/* eth */
	'ñ',	"n~",	/* n tilde */
	'ò',	"o`",	/* o grave */
	'ó',	"o'",	/* o acute */
	'ô',	"o^",	/* o circumflex */
	'õ',	"o~",	/* o tilde */
	'ö',	"o\"",	/* o dieresis */
	'ö',	"o:",	/* o dieresis */
	'ö',	"oe",	/* o dieresis */
	'÷',	"-:",	/* divide sign */
	'ø',	"o/",	/* o slash */
	'ù',	"u`",	/* u grave */
	'ú',	"u'",	/* u acute */
	'û',	"u^",	/* u circumflex */
	'ü',	"u\"",	/* u dieresis */
	'ü',	"u:",	/* u dieresis */
	'ü',	"ue",	/* u dieresis */
	'ý',	"y'",	/* y acute */
	'þ',	"th",	/* thorn */
	'þ',	"p|",	/* thorn */
	'ÿ',	"y\"",	/* y dieresis */
	'ÿ',	"y:",	/* y dieresis */
	0,	0,
};

KIOQ	kbdq;

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

	for(tries = 0; (inb(Status) & Outbusy); tries++)
		if(tries > 1000)
			return -1;
	return 0;
}

/*
 *  wait for input
 */
static int
inready(void)
{
	int tries;

	for(tries = 0; !(inb(Status) & Inready); tries++)
		if(tries > 1000)
			return -1;
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

void
kbdinit(void)
{
	int c;

	initq(&kbdq);
	setvec(Kbdvec, kbdintr);

	/* wait for a quiescent controller */
	while((c = inb(Status)) & (Outbusy | Inready))
		if(c & Inready)
			inb(Data);

 	/* enable kbd xfers and interrupts */
	outb(Cmd, 0x60);
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Data, 0x65);
	if(outready() < 0)
		print("kbd init failed\n");
}

/*
 *  keyboard interrupt
 */
int
kbdintr0(void)
{
	int s, c;
	static int esc1, esc2;
	static int shift;
	static int caps;
	static int ctl;
	static int num;
	static int lstate, k1, k2;
	int keyup;

	/*
	 *  get status
	 */
	s = inb(Status);
	if(!(s&Inready))
		return -1;

	/*
	 *  get the character
	 */
	c = inb(Data);

	/*
	 *  e0's is the first of a 2 character sequence
	 */
	if(c == 0xe0){
		esc1 = 1;
		return 0;
	} else if(c == 0xe1){
		esc2 = 2;
		return 0;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		print("unknown key %ux\n", c|keyup);
		kbdputc(&kbdq, k1);
		return 0;
	}

	if(esc1){
		c = kbtabesc1[c];
		esc1 = 0;
		kbdputc(&kbdq, c);
		return 0;
	} else if(esc2){
		esc2--;
		return 0;
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
		case Shift:
			shift = 0;
			break;
		case Ctrl:
			ctl = 0;
			break;
		}
		return 0;
	}

	/*
 	 *  normal character
	 */
	if(!(c & Spec)){
		if(ctl)
			c &= 0x1f;
		switch(lstate){
		case 1:
			k1 = c;
			lstate = 2;
			return 0;
		case 2:
			k2 = c;
			lstate = 0;
			c = latin1(k1, k2);
			if(c == 0){
				kbdputc(&kbdq, k1);
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
			return 0;
		case Num:
			num ^= 1;
			return 0;
		case Shift:
			shift = 1;
			return 0;
		case Latin:
			lstate = 1;
			return 0;
		case Ctrl:
			ctl = 1;
			return 0;
		}
	}
	kbdputc(&kbdq, c);
	return 0;
}

void
kbdintr(Ureg *ur)
{
	USED(ur);

	while(kbdintr0() == 0)
		;
}
