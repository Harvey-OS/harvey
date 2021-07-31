.i
	a b c d
	s s0 s1
	d0 d1 d2 d3
.b
	and2 and2a and2b
	or2 or2a
/*
#	nand2 nanda nand2b
#	nor2 nor2a nor2b
*/

	and3 and3a and3b and3c
	or3 or3a or3b

	and4b and4c
	or4 or4a

	xor xnor xo1 xo1a xa1 xa1a ax1b

	ao1 ao1a ao1b ao1c
	aoi1a aoi1b aoi1c aoi1d
	ao2 ao2a ao2b ao2c ao2d
	aoi2a aoi3a
	ao3 ao3b ao3c ao4a ao5a
	maj3

	oa1 oa1a oa1b oa1c
	oa2 oa2a
	oa3 oa3a oa4 oa4a oa5
	oai1 oai2a oai3a

	mx2 mx2a mx2b mx2c mx4
.e
	and2 = a & b
	and2a = !a & b
	and2b = !a & !b

	or2 = a | b
	or2a = !a | b

/*#	nand2 = !(a & b)
#	nand2a = !(!a & b)
#	nand2b = !(!a & !b)
#
#	nor2 = !(a | b)
#	nor2a = !(!a | b)
#	nor2b = !(!a | !b)
*/

	and3 = a & b & c
	and3a = !a & b & c
	and3b = !a & !b & c
	and3c = !a & !b & !c

	or3 = a | b | c
	or3a = !a | b | c
	or3b = !a | !b | c

	and4b = !a & !b & c & d
	and4c = !a & !b & !c & d
	or4 = a | b | c | d
	or4a = !a | b | c | d

	xor = a ^ b
	xnor = !(a ^ b)
	xo1 = (a ^ b) | c
	xo1a = !(a ^ b) | c
	xa1 = (a ^ b) & c
	xa1a = !(a ^ b) & c
	ax1b = (!a & !b) ^ c

	ao1 = a & b | c
	ao1a = !a & b | c
	ao1b = a & b | !c
	ao1c = !a & b | !c

	aoi1a = !(!a & b | c)
	aoi1b = !(a & b | !c)
	aoi1c = !((!a & !b) | c)
	aoi1d = !((!a & !b) | !c)

	ao2 = a & b | c | d
	ao2a = !a & b | c | d
	ao2b = !a & !b | c | d
	ao2c = !a & b | !c | d
	ao2d = !a & !b | !c | d
	aoi2a = !(!a & b) | c | d
	aoi3a = !(!a & !b & !c) | !a & !d

	ao3 = !a & b & c | d
	ao3b = !a & !b & c | d
	ao3c = !a & !b & !c | d
	ao4a = !a & b & c | a & c & d
	ao5a = !a & b | a & c | d
	maj3 = a & b | a & c | b & c

	oa1 = (a | b) & c
	oa1a = (!a | b) & c
	oa1b = (a | b) & !c
	oa1c = (!a | b) & !c

	oa2 = (a | b) & (c | d)
	oa2a = (!a | b) & (c | d)

	oa3 = (a | b) & c & d
	oa3a = (a | b) & !c & d
	oa4 = (a | b | c) & d
	oa4a = (!c | a | b) & d
	oa5 = (a | b | c) & (a | d)

	oai1 = !((a | b) & c)
	oai2a = !((a | b | c) & !d)
	oai3a = !((a | b) & !c & !d)

	mx2 = s ? b : a
	mx2a = s ? b : !a
	mx2b = s ? !b : a
	mx2c = s ? !b : !a
	mx4 = (s1 << 1 | s0) {d0, d1, d2, d3}
