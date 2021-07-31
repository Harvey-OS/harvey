#include	<u.h>
#include	<libc.h>
#include	"gen.h"

void
ginit(Posn *posn)
{
	int i, p;

	if(posn->bd[4] == (INIT|BKING) &&
	   posn->bd[60] == (INIT|WKING)) {
		memmove(board, posn->bd, 64);
		bkpos = 4;
		wkpos = 60;
	} else
	for(i=0; i<64; i++) {
		p = posn->bd[i];
		board[i] = p;
		p &= 017;
		if(p == WKING)
			wkpos = i;
		if(p == BKING)
			bkpos = i;
	}
	eppos = posn->epp;
	moveno = posn->mvno;
	mvno = 0;
	mvlist[0] = 0;
	amp = ambuf;
	*amp++ = 0;
	for(i=0; p=posn->mv[i]; i++)
		move(p);
}

void
gstore(Posn *posn)
{

	memmove(posn->mv, mvlist, mvno*sizeof(*mvlist));
	posn->mv[mvno] = 0;
	while(mvno)
		rmove();
	memmove(posn->bd, board, 64);

	posn->epp = eppos;
	posn->mvno = moveno;
	ginit(posn);
}

void
getamv(short *mv)
{

	lmp = mv;
	if(moveno & 1)
		bgen();
	else
		wgen();
	*lmp = 0;
}

void
getmv(short *mv)
{
	int i, f;

	lmp = mv;
	if(moveno & 1)
		bgen();
	else
		wgen();
	*lmp = 0;
	lmp = mv;
	for(i=0; mv[i]; i++) {
		if(moveno & 1) {
			bmove(mv[i]);
			f = wattack(bkpos);
			bremove();
		} else {
			wmove(mv[i]);
			f = battack(wkpos);
			wremove();
		}
		if(f)
			*lmp++ = mv[i];
	}
	*lmp = 0;
}

void
move(int mv)
{

	if(moveno & 1)
		bmove(mv);
	else
		wmove(mv);
	moveno++;
	mvlist[mvno++] = mv;
}

void
qmove(int mov)
{
	int from, to, typ, p;

	from = (mov>>6) & 077;
	to = (mov>>0) & 077;
	typ = (mov>>12) & 017;

	p = board[from] & 017;
	board[to] = p;
	board[from] = 0;

	switch(typ) {

	case TNPRO:
		board[to] = WKNIGHT | (p&BLACK);
		break;

	case TBPRO:
		board[to] = WBISHOP | (p&BLACK);
		break;

	case TRPRO:
		board[to] = WROOK | (p&BLACK);
		break;

	case TQPRO:
		board[to] = WQUEEN | (p&BLACK);
		break;

	case TOO:
		from = to+1;
		to--;
		goto mv;

	case TOOO:
		from = to-2;
		to++;

	mv:
		board[to] = board[from] & 017;
		board[from] = 0;
		break;

	case TENPAS:
		if(p & BLACK)
			to -= 8;
		else
			to += 8;
		board[to] = 0;
		break;
	}
	moveno++;
}

int
rmove(void)
{

	if(mvno <= 0)
		return 0;
	moveno--;
	if(moveno & 1)
		bremove();
	else
		wremove();
	return mvlist[--mvno];
}

int
retract(int m)
{
	int p;

	if(m == 0)
		return 1;
	p = board[(m>>6) & 077] & 7;
	if(p == WPAWN)
		return 1;
	if(p == WKING) {
		p = (m>>12) & 017;
		if(p == TOO || p == TOOO)
			return 1;
	}
	p = board[m & 077] & 7;
	if(p != 0)
		return 1;
	return 0;
}

int
check(void)
{

	if(moveno & 1)
		return !wattack(bkpos);
	return !battack(wkpos);
}

void
wgen(void)
{
	schar *p;
	int i, j, k, d, p0, p1;

	for(i=0; i<64; i++)
	if(board[i])
	switch(board[i]) {

	case INIT|WPAWN:
	case WPAWN:
		d = dir[i];
		if((d&ULEFT) == 0) {
			if(color[board[i-9]] < 0)
			if((d&RANK7) != 0) {
				wcheck(i, i-9, TNPRO);
				wcheck(i, i-9, TBPRO);
				wcheck(i, i-9, TRPRO);
				wcheck(i, i-9, TQPRO);
			} else
				wcheck(i, i-9, TNORM);
			if(eppos == i-1)
				wcheck(i, i-9, TENPAS);
		}
		if((d&URIGHT) == 0) {
			if(color[board[i-7]] < 0)
			if((d&RANK7) != 0) {
				wcheck(i, i-7, TNPRO);
				wcheck(i, i-7, TBPRO);
				wcheck(i, i-7, TRPRO);
				wcheck(i, i-7, TQPRO);
			} else
				wcheck(i, i-7, TNORM);
			if(eppos == i+1)
				wcheck(i, i-7, TENPAS);
		}
		if(board[i-8] == 0) {
			if((d&RANK7) != 0) {
				wcheck(i, i-8, TNPRO);
				wcheck(i, i-8, TBPRO);
				wcheck(i, i-8, TRPRO);
				wcheck(i, i-8, TQPRO);
			} else
				wcheck(i, i-8, TNORM);
			if(d & RANK2)
			if(board[i-16] == 0)
				wcheck(i, i-16, TEPENAB);
		}
		continue;

	case INIT|WKNIGHT:
	case WKNIGHT:
		p = attab;
		goto jump;

	case INIT|WKING:
		if(board[56] == (INIT|WROOK))
		if(board[57] == 0)
		if(board[58] == 0)
		if(board[59] == 0)
		if(battack(58))
		if(battack(59))
		if(battack(60))
		if(i == 60)
			wcheck(60, 58, TOOO);
		if(board[63] == (INIT|WROOK))
		if(board[62] == 0)
		if(board[61] == 0)
		if(battack(62))
		if(battack(61))
		if(battack(60))
		if(i == 60)
			wcheck(60, 62, TOO);

	case WKING:
		p = attab+16;

	jump:
		d = dir[i];
		for(j=8; j--; p+=2)
			if((d&p[0]) == 0)
				wcheck(i, i+p[1], TNORM);
		continue;

	case INIT|WBISHOP:
	case WBISHOP:
		p = attab+16;
		j = 4;
		goto slide;

	case INIT|WROOK:
	case WROOK:
		p = attab+24;
		j = 4;
		goto slide;

	case INIT|WQUEEN:
	case WQUEEN:
		p = attab+16;
		j = 8;

	slide:
		while(j--) {
			p0 = *p++;
			p1 = *p++;
			k = i;
			while((dir[k]&p0) == 0) {
				k += p1;
				if(wcheck(i, k, TNORM))
					break;
			}
		}
		continue;
	}
}

int
wcheck(int from, int to, int typ)
{
	int b;

	b = board[to];
	if(color[b] > 0)
		return 1;
	*lmp++ = (typ<<12) | (from<<6) | to;
	return b;
}

void
wmove(int mov)
{
	int i, from, to;

	from = (mov>>6) & 077;
	to = (mov>>0) & 077;
	eppos = 0;
	*amp++ = AMARK | (from<<5) | board[from];
	if(from == 0 && to == 0)
		return;
	*amp++ = (to<<5) | board[to];
	i = board[from] & 017;
	board[to] = i;
	if(i == WKING)
		wkpos = to;
	board[from] = 0;
	switch((mov>>12)&017) {

	case TNPRO:
		board[to] = WKNIGHT;
		break;

	case TBPRO:
		board[to] = WBISHOP;
		break;

	case TRPRO:
		board[to] = WROOK;
		break;

	case TQPRO:
		board[to] = WQUEEN;
		break;

	case TOO:
		from = to+1;
		to--;
		goto mv;

	case TOOO:
		from = to-2;
		to++;

	mv:
		*amp++ = (from<<5) | board[from];
		*amp++ = (to<<5) | board[to];
		board[to] = board[from] & 017;
		board[from] = 0;
		break;

	case TENPAS:
		to += 8;
		*amp++ = (to<<5) | board[to];
		board[to] = 0;
		break;

	case TEPENAB:
		amp[-1] |= EPMARK;
		eppos = to;
		break;
	}
}

void
wremove(void)
{
	int i, pi;

	do {
		amp--;
		i = (*amp>>5) & 077;
		pi = *amp & 037;
		board[i] = pi;
		if((pi&017) == WKING)
			wkpos = i;
	} while((*amp&AMARK) == 0);
	eppos = 0;
	if(amp[-1] & EPMARK)
		eppos = (amp[-1]>>5) & 077;
}

int
battack(int pos)
{
	schar *p;
	int i, j, p0, p1, b, d;

	p = attab;
	d = dir[pos];

	/* attack by knight */
	for(i=8; i--; p+=2) {
		if((d & p[0]) == 0)
		if((board[pos+p[1]]&017) == BKNIGHT)
			return 0;
	}

	/* attack along diagonal */
	for(i=4; i--;) {
		p0 = *p++;
		p1 = *p++;
		j = pos;
		while((dir[j]&p0) == 0) {
			j += p1;
			b = board[j] & 017;
			if(b)
			if(b == BQUEEN || b == BBISHOP)
				return 0;
			else
				break;
		}
	}

	/* attack at right angles */
	for(i=4; i--;) {
		p0 = *p++;
		p1 = *p++;
		j = pos;
		while((dir[j]&p0) == 0) {
			j += p1;
			b = board[j] & 017;
			if(b)
			if(b == BQUEEN || b == BROOK)
				return 0;
			else
				break;
		}
	}

	/* attack by king */
	p -= 16;
	for(i=8; i--; p+=2)
		if((d & p[0]) == 0)
		if((board[pos+p[1]]&017) == BKING)
			return 0;

	/* attack by pawns */
	if((d & ULEFT) == 0)
	if((board[pos-9]&017) == BPAWN)
		return 0;
	if((d & URIGHT) == 0)
	if((board[pos-7]&017) == BPAWN)
		return 0;

	return 1;
}

void
bgen(void)
{
	schar *p;
	int i, j, k, d, p0, p1;

	for(i=0; i<64; i++)
	if(board[i])
	switch(board[i]) {

	case INIT|BPAWN:
	case BPAWN:
		d = dir[i];
		if((d&DLEFT) == 0) {
			if(color[board[i+7]] > 0)
			if((d&RANK2) != 0) {
				bcheck(i, i+7, TNPRO);
				bcheck(i, i+7, TBPRO);
				bcheck(i, i+7, TRPRO);
				bcheck(i, i+7, TQPRO);
			} else
				bcheck(i, i+7, TNORM);
			if(eppos == i-1)
				bcheck(i, i+7, TENPAS);
		}
		if((d&DRIGHT) == 0) {
			if(color[board[i+9]] > 0)
			if((d&RANK2) != 0) {
				bcheck(i, i+9, TNPRO);
				bcheck(i, i+9, TBPRO);
				bcheck(i, i+9, TRPRO);
				bcheck(i, i+9, TQPRO);
			} else
				bcheck(i, i+9, TNORM);
			if(eppos == i+1)
				bcheck(i, i+9, TENPAS);
		}
		if(board[i+8] == 0) {
			if((d&RANK2) != 0) {
				bcheck(i, i+8, TNPRO);
				bcheck(i, i+8, TBPRO);
				bcheck(i, i+8, TRPRO);
				bcheck(i, i+8, TQPRO);
			} else
				bcheck(i, i+8, TNORM);
			if(d & RANK7)
			if(board[i+16] == 0)
				bcheck(i, i+16, TEPENAB);
		}
		continue;

	case INIT|BKNIGHT:
	case BKNIGHT:
		p = attab;
		goto jump;

	case INIT|BKING:
		if(board[0] == (INIT|BROOK))
		if(board[1] == 0)
		if(board[2] == 0)
		if(board[3] == 0)
		if(wattack(2))
		if(wattack(3))
		if(wattack(4))
		if(i == 4)
			bcheck(4, 2, TOOO);
		if(board[7] == (INIT|BROOK))
		if(board[6] == 0)
		if(board[5] == 0)
		if(wattack(6))
		if(wattack(5))
		if(wattack(4))
		if(i == 4)
			bcheck(4, 6, TOO);

	case BKING:
		p = attab+16;

	jump:
		d = dir[i];
		for(j=8; j--; p+=2)
			if((d&p[0]) == 0)
				bcheck(i, i+p[1], TNORM);
		continue;

	case INIT|BBISHOP:
	case BBISHOP:
		p = attab+16;
		j = 4;
		goto slide;

	case INIT|BROOK:
	case BROOK:
		p = attab+24;
		j = 4;
		goto slide;

	case INIT|BQUEEN:
	case BQUEEN:
		p = attab+16;
		j = 8;

	slide:
		while(j--) {
			p0 = *p++;
			p1 = *p++;
			k = i;
			while((dir[k]&p0) == 0) {
				k += p1;
				if(bcheck(i, k, TNORM))
					break;
			}
		}
		continue;
	}
}

int
bcheck(int from, int to, int typ)
{
	int b;

	b = board[to];
	if(color[b] < 0)
		return 1;
	*lmp++ = (typ<<12) | (from<<6) | to;
	return b;
}

void
bmove(int mov)
{
	int i, from, to;

	from = (mov>>6) & 077;
	to = (mov>>0) & 077;
	eppos = 0;
	*amp++ = AMARK | (from<<5) | board[from];
	if(from == 0 && to == 0)
		return;
	*amp++ = (to<<5) | board[to];
	i = board[from] & 017;
	board[to] = i;
	if(i == BKING)
		bkpos = to;
	board[from] = 0;
	switch((mov>>12)&017) {

	case TNPRO:
		board[to] = BKNIGHT;
		break;

	case TBPRO:
		board[to] = BBISHOP;
		break;

	case TRPRO:
		board[to] = BROOK;
		break;

	case TQPRO:
		board[to] = BQUEEN;
		break;

	case TOO:
		from = to+1;
		to--;
		goto mv;

	case TOOO:
		from = to-2;
		to++;

	mv:
		*amp++ = (from<<5) | board[from];
		*amp++ = (to<<5) | board[to];
		board[to] = board[from] & 017;
		board[from] = 0;
		break;

	case TENPAS:
		to -= 8;
		*amp++ = (to<<5) | board[to];
		board[to] = 0;
		break;

	case TEPENAB:
		amp[-1] |= EPMARK;
		eppos = to;
		break;
	}
}

void
bremove(void)
{
	int i, pi;

	do {
		amp--;
		i = (*amp>>5) & 077;
		pi = *amp & 037;
		board[i] = pi;
		if((pi&017) == BKING)
			bkpos = i;
	} while((*amp&AMARK) == 0);
	eppos = 0;
	if(amp[-1] & EPMARK)
		eppos = (amp[-1]>>5) & 077;
}

wattack(int pos)
{
	schar *p;
	int i, j, p0, p1, b, d;

	p = attab;
	d = dir[pos];

	/* attack by knight */
	for(i=8; i--; p+=2)
		if((d & p[0]) == 0)
		if((board[pos+p[1]]&017) == WKNIGHT)
			return 0;

	/* attack along diagonal */
	i = 4;
	while(i--) {
		p0 = *p++;
		p1 = *p++;
		j = pos;
		while((dir[j]&p0) == 0) {
			j += p1;
			b = board[j]&017;
			if(b)
			if(b == WQUEEN || b == WBISHOP)
				return 0;
			else
				break;
		}
	}

	/* attack at right angles */
	i = 4;
	while(i--) {
		p0 = *p++;
		p1 = *p++;
		j = pos;
		while((dir[j]&p0) == 0) {
			j += p1;
			b = board[j]&017;
			if(b)
			if(b == WQUEEN || b == WROOK)
				return 0;
			else
				break;
		}
	}

	/* attack by king */
	p -= 16;
	for(i=8; i--; p+=2)
		if((d & p[0]) == 0)
		if((board[pos+p[1]]&017) == WKING)
			return 0;

	/* attack by pawns */
	if((d & DLEFT) == 0)
	if((board[pos+7]&017) == WPAWN)
		return 0;
	if((d & DRIGHT) == 0)
	if((board[pos+9]&017) == WPAWN)
		return 0;
	return 1;
}
