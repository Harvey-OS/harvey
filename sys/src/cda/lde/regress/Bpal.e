/*
 * By convention the output enable term for
 * PAL's is 100 + the corresponding pin number.
 * this example includes a .tt line for use by wcheck.
 */
.x	Bpal	PAL16L8A
.tt	iiiiiiiiign222222n2v
.i
	A0 : 1    A1 : 2    A2 : 3    A3 : 4
	A4 : 5    A5 : 6    A6 : 7    A7 : 8
	A8 : 9
.o
	SE+ :     12   RNE+ :    13   TD+ :     14   TU+ :     15
	SFSE :    16   Y5 : 17             BRDY :    19

	e12 = 112 e13 = 113 e14 = 114 e15 = 115
	e16 = 116 e17 = 117           e19 = 119
.f
	cnt = A0 A1 A2 A3 A4
	ardy = A5
	crdy = A6
	flushb- = A8
	flusha- = A7
.e
	tmp = ((cnt == 0) ? ardy ? 1 : 0 :
		(cnt == 6) ? (crdy || !flushb-) ? 1 : 0 : 1 )

	/* shift enable + for major data path, also count enable */
	SE+ = !tmp

	/* random number clock enable - */
	RNE+ = !(!flusha- ? 0 : tmp )

	/* transfer down - for ireg */
	TD+ = !((cnt == 0) && ardy)

	/* transfer up + (invert outside) for oreg<0:3> */
	TU+ = !((cnt == 6) && crdy && flushb-)

	/* shift flush status enable */
	SFSE = !(cnt == 3)

	/* ack- back to ardy  */
	Y5 = !!((cnt == 0) && ardy)

	/* ready to A */
	BRDY = !( (cnt == 0)? 1 : 0)

	e12 = 1	e13 = 1	e14 = 1	e15 = 1
	e16 = 1	e17 = 1	e19 = 1
