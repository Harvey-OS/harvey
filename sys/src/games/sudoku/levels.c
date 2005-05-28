#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>

#include "sudoku.h"

void
consumeline(Biobuf *b)
{
	while(Bgetc(b) != '\n')
		;
}

void
fprettyprintbrd(Cell *board)
{
	int x, y, fd;

	fd = create("/tmp/sudoku-print", OWRITE|OTRUNC, 0600);
	if(fd < 0) {
		perror("can not open save file /tmp/sudoku-save");
		return;
	}

	for(x = 0; x < Brdsize; x++) {
		for(y = 0; y < Brdsize; y++) {
			fprint(fd, " ");
			if(board[y*Brdsize + x].digit == -1)
				fprint(fd, ".");
			else
				fprint(fd, "%d", board[y*Brdsize + x].digit+1);

			if(((x*Brdsize + y + 1) % Brdsize) == 0 || (x*Brdsize + y + 1) == Psize)
				fprint(fd, "\n");

			if(((x*Brdsize + y + 1) % 3) == 0 && ((x*Brdsize + y + 1) % Brdsize) != 0)
				fprint(fd, "|");

			if(((x*Brdsize + y + 1) % 27) == 0 && ((x*Brdsize + y + 1) % Psize) != 0)
				fprint(fd, " -------------------\n");

		}
	}
	close(fd);
}

void
fprintbrd(int fd, Cell *board)
{
	int i;
	
	for(i = 0; i < Psize; i++) {
		if(board[i].digit == -1)
			fprint(fd, ".");
		else
			fprint(fd, "%d", board[i].digit+1);

		if((i + 1) % Brdsize == 0)
			fprint(fd, "\n");
	}
	for(i = 0; i < Psize; i++) {
		fprint(fd, "%d", board[i].solve+1);
		if((i + 1) % Brdsize == 0)
			fprint(fd, "\n");
	}

	close(fd);
}

int
loadlevel(char *name, Cell *board)
{
	Biobuf *b;
	char c;
	int i;
	
	b = Bopen(name, OREAD);
	if(b == nil) {
		fprint(2, "could not open file %s: %r\n", name);
		return -1;
	}

	i = 0;
	while((c = Bgetc(b)) > 0) {
		switch(c) {
		case '.':
			board[i].digit = -1;
			board[i].locked = 0;
			if(++i == 81)
				goto next;
			break;
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
			board[i].digit = c - 0x31;
			board[i].locked = 1;
			if(++i == 81)
				goto next;
			break;
		case '\n':
			break;
		default:
			fprint(2, "unknown character in initial board: %c\n", c);
			goto done;
		}
	}
next:
	i = 0;
	while((c = Bgetc(b)) > 0) {
		switch(c) {
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
			board[i].solve = c - 0x31;
			if(++i == 81)
				goto done;
			break;
		case '\n':
			break;
		default:
			fprint(2, "unknown character in board solution: %c\n", c);
			goto done;
		}
	}

done:
	Bterm(b);

	return i < 81 ? 0 : 1;
}

void
printboard(Cell *board)
{
	int fd;
	
	fd = create("/tmp/sudoku-board", OWRITE|OTRUNC, 0600);
	if(fd < 0) {
		perror("can not open save file /tmp/sudoku-save");
		return;
	}

	fprintbrd(fd, board);

	close(fd);
}

void
savegame(Cell *board)
{
	int fd;
	
	fd = create("/tmp/sudoku-save", OWRITE|OTRUNC, 0600);
	if(fd < 0) {
		perror("can not open save file /tmp/sudoku-save");
		return;
	}

	if(write(fd, board, Psize * sizeof(Cell)) != Psize * sizeof(Cell)) {
		perror("could not save to file");
		close(fd);
	}

	close(fd);
}

int
loadgame(Cell *board)
{
	int fd;

	fd = open("/tmp/sudoku-save", OREAD);
	if(fd < 0) {
		perror("can not open save file /tmp/sudoku-save");
		return -1;
	}

	if(read(fd, board, Psize * sizeof(Cell)) != Psize * sizeof(Cell)) {
		perror("insufficient data in save file");
		close(fd);
		return -1;
	}
	
	close(fd);

	return 1;
}
