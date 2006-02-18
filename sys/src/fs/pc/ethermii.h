typedef struct Mii Mii;
typedef struct MiiPhy MiiPhy;

enum {					/* registers */
	Bmcr		= 0x00,		/* Basic Mode Control */
	Bmsr		= 0x01,		/* Basic Mode Status */
	Phyidr1		= 0x02,		/* PHY Identifier #1 */
	Phyidr2		= 0x03,		/* PHY Identifier #2 */
	Anar		= 0x04,		/* Auto-Negotiation Advertisement */
	Anlpar		= 0x05,		/* AN Link Partner Ability */
	Aner		= 0x06,		/* AN Expansion */
	Annptr		= 0x07,		/* AN Next Page TX */
	Annprr		= 0x08,		/* AN Next Page RX */
	Mscr		= 0x09,		/* MASTER-SLAVE Control */
	Mssr		= 0x0A,		/* MASTER-SLAVE Status */
	Esr		= 0x0F,		/* Extended Status */

	NMiiPhyr	= 32,
	NMiiPhy		= 32,
};

enum {					/* Bmcr */
	BmcrSs1		= 0x0040,	/* Speed Select[1] */
	BmcrCte		= 0x0080,	/* Collision Test Enable */
	BmcrDm		= 0x0100,	/* Duplex Mode */
	BmcrRan		= 0x0200,	/* Restart Auto-Negotiation */
	BmcrI		= 0x0400,	/* Isolate */
	BmcrPd		= 0x0800,	/* Power Down */
	BmcrAne		= 0x1000,	/* Auto-Negotiation Enable */
	BmcrSs0		= 0x2000,	/* Speed Select[0] */
	BmcrLe		= 0x4000,	/* Loopback Enable */
	BmcrR		= 0x8000,	/* Reset */
};

enum {					/* Bmsr */
	BmsrEc		= 0x0001,	/* Extended Capability */
	BmsrJd		= 0x0002,	/* Jabber Detect */
	BmsrLs		= 0x0004,	/* Link Status */
	BmsrAna		= 0x0008,	/* Auto-Negotiation Ability */
	BmsrRf		= 0x0010,	/* Remote Fault */
	BmsrAnc		= 0x0020,	/* Auto-Negotiation Complete */
	BmsrPs		= 0x0040,	/* Preamble Suppression Capable */
	BmsrEs		= 0x0100,	/* Extended Status */
	Bmsr100T2HD	= 0x0200,	/* 100BASE-T2 HD Capable */
	Bmsr100T2FD	= 0x0400,	/* 100BASE-T2 FD Capable */
	Bmsr10THD	= 0x0800,	/* 100BASE-T HD Capable */
	Bmsr10TFD	= 0x1000,	/* 10BASE-T FD Capable */
	Bmsr100TXHD	= 0x2000,	/* 100BASE-TX HD Capable */
	Bmsr100TXFD	= 0x4000,	/* 100BASE-TX FD Capable */
	Bmsr100T4	= 0x8000,	/* 100BASE-T4 Capable */
};

enum {					/* Anar/Anlpar */
	Ana10HD		= 0x0020,	/* Advertise 10BASE-T */
	Ana10FD		= 0x0040,	/* Advertise 10BASE-T FD */
	AnaTXHD		= 0x0080,	/* Advertise 100BASE-TX */
	AnaTXFD		= 0x0100,	/* Advertise 100BASE-TX FD */
	AnaT4		= 0x0200,	/* Advertise 100BASE-T4 */
	AnaP		= 0x0400,	/* Pause */
	AnaAP		= 0x0800,	/* Asymmetrical Pause */
	AnaRf		= 0x2000,	/* Remote Fault */
	AnaAck		= 0x4000,	/* Acknowledge */
	AnaNp		= 0x8000,	/* Next Page Indication */
};

enum {					/* Mscr */
	Mscr1000THD	= 0x0100,	/* Advertise 1000BASE-T HD */
	Mscr1000TFD	= 0x0200,	/* Advertise 1000BASE-T FD */
};

enum {					/* Mssr */
	Mssr1000THD	= 0x0400,	/* Link Partner 1000BASE-T HD able */
	Mssr1000TFD	= 0x0800,	/* Link Partner 1000BASE-T FD able */
};

enum {					/* Esr */
	Esr1000THD	= 0x1000,	/* 1000BASE-T HD Capable */
	Esr1000TFD	= 0x2000,	/* 1000BASE-T FD Capable */
	Esr1000XHD	= 0x4000,	/* 1000BASE-X HD Capable */
	Esr1000XFD	= 0x8000,	/* 1000BASE-X FD Capable */
};

typedef struct Mii {
	Lock;
	int	nphy;
	int	mask;
	MiiPhy*	phy[NMiiPhy];
	MiiPhy*	curphy;

	void*	ctlr;
	int	(*mir)(Mii*, int, int);
	int	(*miw)(Mii*, int, int, int);
} Mii;

typedef struct MiiPhy {
	Mii*	mii;
	int	oui;
	int	phyno;

	int	anar;
	int	fc;
	int	mscr;

	int	link;
	int	speed;
	int	fd;
	int	rfc;
	int	tfc;
};

extern int mii(Mii*, int);
extern int miiane(Mii*, int, int, int);
extern int miimir(Mii*, int);
extern int miimiw(Mii*, int, int);
extern int miireset(Mii*);
extern int miistatus(Mii*);
