#include <u.h>
#include <libc.h>
#include <draw.h>

#include "sudoku.h"

int allowbits[Brdsize] = {
	0x00100,
	0x00200,
	0x00400,
	0x00800,
	0x01000,
	0x02000,
	0x04000,
	0x08000,
	0x10000
};

int boxind[Brdsize][Brdsize] = {
	{0,1,2,9,10,11,18,19,20},
	{3,4,5,12,13,14,21,22,23},
	{6,7,8,15,16,17,24,25,26},
	{27,28,29,36,37,38,45,46,47},
	{30,31,32,39,40,41,48,49,50},
	{33,34,35,42,43,44,51,52,53},
	{54,55,56,63,64,65,72,73,74},
	{57,58,59,66,67,68,75,76,77},
	{60,61,62,69,70,71,78,79,80},
};
int colind[Brdsize][Brdsize] = {
	{0,9,18,27,36,45,54,63,72},
	{1,10,19,28,37,46,55,64,73},
	{2,11,20,29,38,47,56,65,74},
	{3,12,21,30,39,48,57,66,75},
	{4,13,22,31,40,49,58,67,76},
	{5,14,23,32,41,50,59,68,77},
	{6,15,24,33,42,51,60,69,78},
	{7,16,25,34,43,52,61,70,79},
	{8,17,26,35,44,53,62,71,80},
};
int rowind[Brdsize][Brdsize] = {
	{0,1,2,3,4,5,6,7,8},
	{9,10,11,12,13,14,15,16,17},
	{18,19,20,21,22,23,24,25,26},
	{27,28,29,30,31,32,33,34,35},
	{36,37,38,39,40,41,42,43,44},
	{45,46,47,48,49,50,51,52,53},
	{54,55,56,57,58,59,60,61,62},
	{63,64,65,66,67,68,69,70,71},
	{72,73,74,75,76,77,78,79,80},
};

static int maxlevel;
static int solved;

void
printbrd(int *board)
{
	int i;

	for(i = 0; i < Psize; i++) {
		if(i > 0 && i % Brdsize == 0) 
			print("\n");
		print("%2.2d ", board[i] & Digit);
	}
	print("\n");
}

int
getrow(int cell)
{
	return cell/9;
}

int
getcol(int cell)
{
	return cell%9;
}

int
getbox(int cell)
{
	int row = getrow(cell);
	int col = getcol(cell);

	return 3*(row/3)+ col/3;
}

void
setdigit(int cc, int num)
{
	board[cc] = (board[cc] & ~Digit) | num;
}

int
boxcheck(int *board)
{
	int i,j,d,sum,last,last2;

	for (i = 0; i < 9; i++) {
		for (d = 0;d < 9; d++) {
			sum=0;
			last=-1;
			last2=-1;
			for (j = 0; j < 9; j++) {
				if (board[boxind[i][j]] & allowbits[d]) {
					sum++;
					last2=last;
					last=boxind[i][j];
				} else
					sum += ((board[boxind[i][j]] & Solve)==(d << 4)) ? 1: 0;
			}
			if (sum==0) 
				return(0);
			if ((sum==1)&&(last>=0))
				if (!setallowed(board,last,d)) 
					return(0);

			if((sum == 2) && (last >= 0) && ( last2 >= 0) && 
					(getrow(last) == getrow(last2))) {
				for (j = 0; j < 9; j++) {
					int c = rowind[getrow(last)][j];
					if ((c != last)&&(c != last2)) {
						if (board[c] & allowbits[d]) {
							board[c] &= ~allowbits[d];
							if ((board[c] & Allow)==0) 
								return(0);
						}
					}
				}
			}
			if((sum == 2) && (last >= 0) && (last2 >= 0) &&
					(getcol(last) == getcol(last2))) {
				for (j = 0;j  <9;j++) {
					int c = colind[getcol(last)][j];
					if ((c != last) && (c != last2)) {
						if (board[c] & allowbits[d]) {
							board[c] &= ~allowbits[d];
							if ((board[c] & Allow) == 0) 
								return(0);
						}
					}
				}
			}
		}
	}
	return(1);
}

int
rowcheck(int *board)
{
	int i,j,d,sum,last;

	for (i = 0; i < 9; i++) {
		for (d = 0; d < 9; d++) {
			sum = 0;
			last = -1;
			for (j = 0; j <9 ; j++) {
				if (board[rowind[i][j]] & allowbits[d]) {
					sum++;
					last = j;
				} else
					sum += ((board[rowind[i][j]] & Solve) == (d << 4)) ? 1: 0;
			}
			if (sum == 0) 
				return(0);
			if ((sum == 1) && (last >= 0)) {
				if (!setallowed(board, rowind[i][last], d)) 
						return(0);
			}
		}
	}
	return(1);
}

int
colcheck(int *board)
{
	int i,j,d,sum,last;

	for (i = 0; i < 9; i++) {
		for (d = 0; d < 9; d++) {
			sum = 0;
			last = -1;
			for (j = 0;j < 9;j++) {
				if (board[colind[i][j]] & allowbits[d]) {
					sum++;
					last = j;
				} else
					sum += ((board[colind[i][j]] & Solve) == (d << 4)) ? 1: 0;
			}
			if (sum == 0) 
				return(0);
			if ((sum == 1) && (last >= 0)) {
				if (!setallowed(board, colind[i][last], d)) 
					return(0);
			}
		}
	}
	return(1);
}

int
setallowed(int *board, int cc, int num)
{
	int j, d;
	int row, col, box;

	board[cc] &= ~Allow;
	board[cc] = (board[cc] & ~Solve) | (num << 4);

	row = getrow(cc);
	for (j = 0; j < 9; j++) {
		if (board[rowind[row][j]] & allowbits[num]) {
			board[rowind[row][j]] &= ~allowbits[num];
			if ((board[rowind[row][j]] & Allow) == 0) 
				return(0);
		}
	}

	col = getcol(cc);
	for (j = 0; j < 9; j++) {
		if (board[colind[col][j]] & allowbits[num]) {
			board[colind[col][j]] &= ~allowbits[num];
			if ((board[colind[col][j]] & Allow) == 0) 
				return(0);
		}
	}

	box = getbox(cc);
	for (j = 0;j < 9;j++) {
		if (board[boxind[box][j]] & allowbits[num]) {
			board[boxind[box][j]] &= ~allowbits[num];
			if ((board[boxind[box][j]] & Allow)==0) 
				return(0);
		}
	}

	for (j = 0;j < 81; j++)
		for (d = 0; d < 9; d++)
			if ((board[j] & Allow) == allowbits[d])
				if (!setallowed(board, j, d)) 
					return(0);

	if (!boxcheck(board)||!rowcheck(board)||!colcheck(board))
		return(0);

	for (j = 0; j < 81; j++)
		for (d = 0; d < 9; d++)
			if ((board[j] & Allow) == allowbits[d])
				if (!setallowed(board, j, d)) 
					return(0);

	return(1);
}

int
chksolved(int *board)
{
	int i;

	for (i = 0; i < Psize; i++)
		if ((board[i] & Allow) != 0)
			return 0;

	solved = 1;
	return solved;
}

int
findmove(int *board)
{
	int i, d;
	int s;

	s = nrand(Psize);
	for (i=(s+1)%81;i!=s;i=(i+1)%81) {
		if (!(board[i] & Allow)) {
			d=(board[i] & Solve) >> 4;
			if ((board[i] & Digit)!=d) 
				return(i);
		}
	}
	return(-1);
}

void
attempt(int *pboard, int level)
{
	int tb[Psize];
	int i, j, k;
	int s, e;

	if (level > maxlevel)
		maxlevel = level;

	if (level > 25) 
		exits("level");	/* too much */

	s = nrand(Psize);
	for (i = (s + 1) % Psize; i != s; i = (i + 1) % Psize) {
		if ((pboard[i] & Allow) != 0) {
			e=nrand(9);
			for (j = (e + 1) % 9; j != e; j = (j + 1) % 9) {
				if (pboard[i] & allowbits[j]) {
					for (k = 0; k < Psize; k++) 
						tb[k] = pboard[k];

					if (setallowed(tb, i, j)) {
						tb[i] = (tb[i] & ~Digit) | j;
						if (chksolved(tb)) {
							for (k = 0;k < Psize; k++) 
								pboard[k] = tb[k];
							return;	/* bad! */
						}

						attempt(tb, level + 1);
						if (chksolved(tb)) {
							for (k = 0; k < Psize; k++) 
								pboard[k] = tb[k];
							return;
						}
						tb[i] |= Digit;
						if (level > 25) 
							return;
					}
				}
			}
		}
	}
}

void
solve(void)
{
	int pboard[Psize];
	int	i, c;

	if (!solved) {
		for (i = 0; i < Psize; i++) 
			pboard[i] = Allow | Solve | Digit;

		for (i = 0; i < Psize; i++) {

			c = board[i] & Digit;
			if ((0 <= c) && (c < 9)) {
				if (!setallowed(pboard,i,c)) {
					print("starting position impossible... failed at cell %d char: %d\n", i, c + 1);
					return;
				}
			}
		}
		attempt(pboard,0);

		for (i = 0; i < Psize; i++)
			board[i] = (board[i] & ~Solve) | (pboard[i] & Solve);
	}
}

int
checklegal(int cc, int d)
{
	int hold = board[cc];
	int j, row, col, box;
	board[cc] |= Digit;

	row = getrow(cc);
	for (j = 0; j < Brdsize; j++)
		if ((board[rowind[row][j]] & Digit) == d) {
			board[cc] = hold;
			return(0);
		}

	col = getcol(cc);
	for (j = 0; j < Brdsize; j++)
		if ((board[colind[col][j]] & Digit) == d) {
			board[cc] = hold;
			return(0);
		}

	box = getbox(cc);
	for (j = 0; j < Brdsize; j++)
		if ((board[boxind[box][j]] & Digit) == d) {
			board[cc] = hold;
			return(0);
		}

	board[cc]=hold;
	return(1);
}

void
clearp(void)
{
	int i;
	for(i = 0; i < Psize; i++) {
		board[i] = (Allow | Solve | Digit);
	}
	solved = 0;
}

void
makep(void)
{
	int i,d;

	do {
		clearp();
		maxlevel=0;

		for (d = 0; d < Brdsize; d++) {
			i = nrand(Psize);
			if (board[i] & allowbits[d]) {
				setallowed(board, i, d);
				board[i] = (board[i] & ~Digit) | d;
			}
		}

		attempt(board, 0);

		for (i = 0; i < Psize; i++) {
			if ((0 <= (board[i] & Digit)) && ((board[i] & Digit) < 9))
				board[i] |= MLock;
			setdigit(i, board[i] & Digit);
		}

		if (!solved) {
			fprint(2, "failed to make puzzle\n");
			exits("failed to make puzzle");
		}

	} while (!solved);

}
