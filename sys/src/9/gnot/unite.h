/* AT&T T7250A User Network Interface for Terminal Equipment (UNITE) */

enum Unite_r0 {		/* Chip/Hardware Configuration Register */
	Ckdm	= Bit(1),
	C2pol	= Bit(2),
	C1pol	= Bit(3),
	Dynrd	= Bit(4),
};

enum Unite_r1 {		/* Line Interface Status */
	Prss	= Bits(0,1),
	Rss	= Bits(2,3),
};

enum Unite_r2 {		/* Transmitter Control Register */
	Tss	= Bits(0,1),
	Tabt	= Bit(2),
	Tfc	= Bit(3),
	Tinv	= Bit(4),
	Rinv	= Bit(5),
	Ent	= Bit(6),
	Enr	= Bit(7)
};

enum Unite_r4 {		/* Hardware Configuration Register */
	Tpol	= Bit(0),
	Ipol	= Bit(1),
	Clkmux	= Bit(2),
	B2f	= Bit(3),
	B1f	= Bit(4),
	Syscko	= Bit(5),
	Codec2	= Bit(6),
	Codec1	= Bit(7)
};

enum Unite_r5 {		/* HDLC Receiver Status */
	Rqs	= Bits(0,4),
	Overrun	= Bit(5),
	Ridl	= Bit(6),
	Eof	= Bit(7)
};

enum Unite_r6 {		/* Interrupt Trigger Levels for D-Channel Queues */
	Tel	= Bits(0,3),
	Rfl	= Bits(4,7)
};

enum Unite_r7 {		/* Multiframe Status and S-Channel Queues */
	Sdata	= Bits(0,4),
	Mstate	= Bits(5,6),
	Mse	= Bit(7)
};

enum Unite_r8 {		/* S/T Interface control */
	Clrmm	= Bit(0),
	B1xb2	= Bit(1),
	Pry	= Bit(2),
	Prye	= Bit(3),
	Lpry	= Bit(4),
	Ssm	= Bit(5),
	B2i	= Bit(6),
	B1i	= Bit(7)
};

enum Unite_r9 {		/* Interrupt masks */
	Mmie	= Bit(0),
	Richgie	= Bit(1),
	Qdie	= Bit(2),
	Ssie	= Bit(3),
	Tdie	= Bit(4),
	Teie	= Bit(5),
	Rfie	= Bit(6),
	Reofie	= Bit(7)
};

enum Unite_r10 {	/* Interrupt Status */
	Mm	= Bit(0),
	Richg	= Bit(1),
	Qdone	= Bit(2),
	Ss	= Bit(3),
	Tdone	= Bit(4),
	Tempty	= Bit(5),
	Rfull	= Bit(6),
	Reof	= Bit(7)
};

enum Unite_r11 {	/* Q-Channel Data and Status */
	Qdata	= Bits(0,3),
	Enflg	= Bit(4),
	Doneq	= Bit(5),
	Qse	= Bit(7)
};

enum Unite_r12 {	/* Loopback and Transmit 1's Control */
	Rlb2	= Bit(0),
	Rlb1	= Bit(1),
	Rld	= Bit(2),
	Llb2	= Bit(3),
	Llb1	= Bit(4),
	Lld	= Bit(5),
	T1b2	= Bit(6),
	T1b1	= Bit(7)
};

enum Unite_r13 {	/* Timer Configuration Control */
	Tim	= Bits(0,3),
	Tm	= Bits(4,7)
};

enum Unite_r14 {	/* Transmitter Queue Status */
	Tqs	= Bits(0,4),
	Undabt	= Bit(5),
};

enum Unite_r15 {	/* Software Resets */
	Mres	= Bit(0),
	Rres	= Bit(1),
	Tres	= Bit(2),
	Tcrcb	= Bit(4),
};
