/*
 * An example using parameter passing and Don't_care
 */
.x	dram	PAL16R6
.tt	iiiiiinnngin222222nv
.i
	CK:1 OE-:11
	dreq:2 stall:3 cerr:4 read:5 qword:6
	rasefb=18 casxfb=17 casyfb=16 wefb=15
	dsfb0=14 dsfb1=13
.o
	rase:18 casx:17 casy:16 we:15
	ds0:14
	ds1:13
.f
	DS = rase casx casy we ds1 ds0
	DSfb = rasefb casxfb casyfb wefb dsfb1 dsfb0
.u
	CK	OE-
.e
	X.NCAS = 0100       /* don't care bits  */

	DC =	0200        /* don't care state */
	S.RAS =	040
	S.CAS =	020
	S.NCAS = 010
	S.WE =	004

	/* low order 2 bits of state vector     */
	A = $0    B = $1    C = $2    D = $3

	 I0 = C        /* state assignement     */
	D10 = S.RAS + A
	D11 = S.RAS + S.CAS + X.NCAS + B
	D12 = S.RAS + S.CAS + X.NCAS + A
	D13 = S.RAS +         S.NCAS + B
	D23 = S.RAS + B
	D14 = S.RAS +         S.NCAS + D
	D24 = S.RAS + D
	D15 = S.RAS +         S.NCAS + C
	D25 = B
	D16 = A
	D26 = D
	D31 = S.RAS + S.CAS + X.NCAS        + D
	D32 = S.RAS + S.CAS + X.NCAS + S.WE + A
	D33 = S.RAS +         S.NCAS + S.WE + B
	D43 = S.RAS + S.CAS + X.NCAS + S.WE + B
	D34 = S.RAS +         S.NCAS + S.WE + D
	D44 = S.RAS + S.CAS + X.NCAS + S.WE + D
	D35 = S.RAS +         S.NCAS + S.WE + C
	D36 = S.WE

	DScom = DSfb {
		  I0: dreq ? D10 : I0,         /* idle state  */
		 D10: read ? D11 : D31,
		 D31: stall ? D31 : D32,
		 D32: qword ? (stall ? D32 : D33) : D36,
		 D33: stall ? D43 : D34,
		 D43: stall ? D43 : D34,
		 D34: stall ? D44 : D35,
		 D44: stall ? D44 : D35,
		 D35: D36,
		 D36: I0,
		 D11: stall ? D11 : D12,
		 D12: qword ? D13 : D16,
		 D13: cerr ? D23 : D14,
		 D23: D14,
		 D14: cerr ? D24 : D15,
		 D24: D15,
		 D15: cerr ? D25 : D16,
		 D25: D16,
		 D16: cerr ? D26 : I0,
		 D26: dreq ? D10 : I0,
		    : DC
         }

	DS = 077 ^ DScom
	DS' = (DScom == DC ) ? ~0 :
		((DScom & X.NCAS) ? S.NCAS : 0)
