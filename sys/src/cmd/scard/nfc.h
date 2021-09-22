typedef struct Atq Atq;


enum {
	Maxuid = 64,

	/* Card name cointained in Sel_res*/
	MifUlite = 0x00,	/*  Mifare Ultralight */
	Mif1k = 0x08,	/*  Mifare 1k */
	Mifmini = 0x09,	/* Mifare Mini */
	Mif4k = 0x18,	/*  Mifare 4k*/
	MifDesfire = 0x20,	/* Mifare Desfire */
	Jcop30	= 0x28,
	MifClass = 0x88,	/* Mifare classic */
	GemPlus = 0x98,
	MaxCard,
};



struct Atq{
	uchar ntagfnd;
	uchar tagn;
	uchar alen;
	ushort sensres;
	ushort selres;
	uchar *uid;
	uchar nuid;
	uchar packuid[Maxuid];
};
char *		atq2str(Atq *a);
int		unpackatq(uchar *b, int bsz, Atq *a);
