.x
	IOCTL
.i
	CK
	PORT	BWE-	BDS-
	BA0	BA1	BA2	BA3	BA4	BA5
	A0	A1
.io
	S0	S1	S2
	RE-	WE-
	UNITE-	HDLC0-	HDLC1-
.o
	BSEL-	AWS-	SOE-	BACK
.f
	S    = S0 S1 S2	/* 7, 6, 4, 5, 1, 0, 2, 3 */
	SYNC = S0 S1 S2	UNITE- HDLC0- HDLC1-
	BA   = BA0 BA1 BA2 BA3 BA4 BA5
	A    = A0 A1
.e
	S ~= ~((S) { 2, 0, 3, 3, 5, 1, 4, (BDS- ? 7 : 6) })

	t0=7	t1=6	t2=4	t3=5	t4=1	t5=0	t6=2	t7=3

	Track = (S==t6) | (S==t7)
	Twack = (S==t4) | (S==t5) | Track
	Tws   = (S==t3) | Twack

	asr  = PORT & (BA == 064)
	data = PORT & (BA == 065)

	BSEL- ~= asr | data

	AWS- ~= asr & !BWE- & !BDS-
	SOE- ~= asr &  BWE-

	BACK ~= !((asr & !BDS-) | (data & ((!BWE- & Twack) | Track)))

	rwgate = data & (!UNITE- | !HDLC0- | !HDLC1-)

	WE- ~= rwgate & !BWE- & !Tws
	RE- ~= rwgate & BWE-

	latch = !RE- | !WE-

	UNITE- ~= !UNITE- | (data & A == 0 & !latch)
	HDLC0- ~= !HDLC0- | (data & A == 2 & !latch)
	HDLC1- ~= !HDLC1- | (data & A == 3 & !latch)

	SYNC := 077*CK
	SYNC -= 077*(!PORT & !latch)
	SYNC += 0
