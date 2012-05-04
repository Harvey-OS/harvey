struct Ureg {
	u64int	ax;
	u64int	bx;
	u64int	cx;
	u64int	dx;
	u64int	si;
	u64int	di;
	u64int	bp;
	u64int	r8;
	u64int	r9;
	u64int	r10;
	u64int	r11;
	u64int	r12;
	u64int	r13;
	u64int	r14;
	u64int	r15;

	u16int	ds;
	u16int	es;
	u16int	fs;
	u16int	gs;

	u64int	type;
	u64int	error;		/* error code (or zero) */
	u64int	ip;		/* pc */
	u64int	cs;		/* old context */
	u64int	flags;		/* old flags */
	u64int	sp;		/* sp */
	u64int	ss;		/* old stack segment */
};
