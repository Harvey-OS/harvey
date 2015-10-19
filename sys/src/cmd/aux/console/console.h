/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IODATASZ	8192

enum
{
	KeyboardBufferSize = 32,
	ScreenBufferSize = KeyboardBufferSize*8,
};

extern int stdio;
extern int blind;	/* no feedback for input, disable rawmode */

extern void debug(const char *, ...);

extern int fsinit(int *, int *);
extern void fsserve(int, char*);

extern void writecga(int, int);

extern void readkeyboard(int, int);

typedef struct Buffer Buffer;
struct Buffer
{
	uint32_t size;
	uint32_t written;
	uint32_t read;
	int flushlines;
	char *data;
};
extern Buffer* balloc(uint32_t);
extern uint32_t bwrite(Buffer *, char *, uint32_t);
extern void bdelete(Buffer *, uint32_t);
extern char *bread(Buffer *, uint32_t *);
#define bempty(b) (b->read == b->written)
