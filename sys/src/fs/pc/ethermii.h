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
	Gbtcr		= 0x09,		/* 1000BASE-T Control */
	Gbtsr		= 0x0A,		/* 1000BASE-T Status */
	Gbscr		= 0x0F,		/* 1000BASE-T Extended Status */

	NMiiPhyr	= 32,
	NMiiPhy		= 32,
};

typedef struct Mii {
	Lock;
	int	nphy;
	int	mask;
	MiiPhy*	phy[NMiiPhy];
	MiiPhy*	curphy;

	void*	ctlr;
	int	(*mir)(Mii*, int, int);
	void	(*miw)(Mii*, int, int, int);
} Mii;

typedef struct MiiPhy {
	Mii*	mii;
	int	oui;
	int	phy;

	ushort	r[NMiiPhyr];

	int	valid;
	int	link;
	int	speed;
	int	duplex;
	int	pause;
};

extern int mii(Mii*, int);
