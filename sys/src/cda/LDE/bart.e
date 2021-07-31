.i
	clk
	req-
	mine-
.b
	tick[6]
	count[4]
	t[8]
.o
	csync-
	ack-
	sel-
	busy-
	phase
	osc
.e
	tick = tick ^ ((tick == 39) ? 0 : (tick + 1))
	tick ^= clk ? ~0 : 0
	count = count ^ ((tick == 39) ? ((count == 9) ? 0 : (count + 1)) : count)
	count ^= clk ? ~0 : 0
	csync- ~= (count) {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}	/* h/2 */
/*	csync- ~= (count) {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0}	/* neg */
	ack- ~= req
	t = 1 | (t << 1)
	t := clk ? ~0 : 0
	sel- ~= t3 & !t7
	busy- ~= t1 & !t5 | mine
/*	sd6- ~= 1		/* force selection */
	phase = clk
