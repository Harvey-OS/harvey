.i
	a b c d e
	s s0 s1
	d0 d1 d2 d3
	a0 a1 b0 b1
.b
	and4 and4a and5b
	or2b or3c or4b or4c or5b

	ax1 ax1c
	ao2e ao3a ao6 ao6a ao7 ao8 ao9 ao10

	aoi1 aoi2b aoi4a
	oa3b oai3

	cs1 cy2a
.e
	and4 = a & b & c & d
	and4a = !a & b & c & d
	and5b = !a & !b & c & d & e
	or2b = !a | !b
	or3c = !a | !b | !c
	or4b = !a | !b | c | d
	or4c = !a | !b | !c | d
	or5b = !a | !b | c | d | e

	ax1 = (!a & b) ^ c
	ax1c = (a & b) ^ c
	ao2e = !a & !b | !c | !d
	ao3a = a & b & c | d
	ao6 = a & b | c & d
	ao6a = a & b | !d & c
	ao7 = a & b & c | d | e
	ao8 = a & b | !c & !d | e
	ao9 = a & b | c | d | e
	ao10 = (a & b | c) & (d | e)

	aoi1 = !(a & b | c)
	aoi2b = !(!a & b | !c | d)
	aoi4a = !(a & b | !c & d)
	oa3b = (!a | b) & !c & d
	oai3 = !((a | b) & c & d)

	cs1 = !(a | s & b) & c | d & (a | s & b)
	cy2a = a1 & b1 | a0 & b0 & a1 | a0 & b0 & b1
