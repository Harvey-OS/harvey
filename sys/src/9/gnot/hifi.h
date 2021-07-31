/* AT&T T7121 HDLC Interface for ISDN (HIFI-64) */

enum Hifi_r0 {		/* Chip Configuration Register */
	Dint	= Bit(0),
	Ipol	= Bit(1),
	Flags	= Bit(2),
	Bm	= Bit(3),
	Alt	= Bit(4),
	Fe	= Bit(5),
	Fspol	= Bit(6),
	Fsen	= Bit(7)
};

enum Hifi_r1 {		/* Transmitter Control Register */
	Til	= Bits(0,5),
	Tabt	= Bit(6),
	Tfc	= Bit(7)
};

enum Hifi_r2 {		/* Transmitter Status Register */
	Tqs	= Bits(0,6),
	Ted	= Bit(7)
};

enum Hifi_r4 {		/* Receiver Status Register */
	Rqs	= Bits(0,6),
	Eof	= Bit(7)
};

enum Hifi_r5 {		/* Receiver Control Register */
	Ril	= Bits(0,5),
	P21ctl	= Bit(6),
	P17ctl	= Bit(7)
};

enum Hifi_r6 {		/* Operation Control Register */
	Rloop	= Bit(0),
	Lloop	= Bit(1),
	Enr	= Bit(2),
	Ent	= Bit(3),
	Rres	= Bit(4),
	Tres	= Bit(5),
	Tristate= Bit(6),
	Pdwn	= Bit(7)
};

enum Hifi_r7 {		/* Transmit Time Slot Control Register */
	Tslt	= Bits(0,5),
	Dxbc	= Bit(6),
	Dxac	= Bit(7)
};

enum Hifi_r8 {		/* Receiver Time Slot Control Register */
	Rslt	= Bits(0,5),
	Iom2	= Bit(6),
	Drab	= Bit(7)
};

enum Hifi_r9 {		/* Bit Offset Control Register */
	Clkri	= Bit(0),
	Rbof	= Bits(1,3),
	Clkxi	= Bit(4),
	Tbof	= Bits(5,7),
};

enum Hifi_r10 {		/* Transmitter Time Slot Offset Control Register */
	Ttsof	= Bits(0,5),
	Tlbit	= Bit(6),
	Dxi	= Bit(7)
};

enum Hifi_r11 {		/* Receiver Time Slot Offset Control Register */
	Rtsof	= Bits(0,5),
	Rlbit	= Bit(6),
	Dri	= Bit(7)
};

enum Hifi_ar11 {	/* Transparent Mode Control Register */
	Octof	= Bits(0,2),
	Mstat	= Bit(3),
	Aloct	= Bit(4),
	Match	= Bit(5),
	Trans	= Bit(6),
	Test	= Bit(7)
};

enum Hifi_r14 {		/* Interrupt Mask Register */
	Tdie	= Bit(0),
	Teie	= Bit(1),
	Undie	= Bit(2),
	Rfie	= Bit(3),
	Reofie	= Bit(4),
	Rovie	= Bit(5),
	Riie	= Bit(6),
	Tbcrc	= Bit(7)
};

enum Hifi_r15 {		/* Interrupt Status Register */
	Tdone	= Bit(0),
	Te	= Bit(1),
	Undabt	= Bit(2),
	Rf	= Bit(3),
	Reof	= Bit(4),
	Overun	= Bit(5),
	Ridl	= Bit(6)
};
