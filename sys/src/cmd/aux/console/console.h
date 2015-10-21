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

#define IODATASZ	8192	/* do not change this, or convM2S wont work! */

enum
{
	KeyboardBufferSize = 32,
	ScreenBufferSize = KeyboardBufferSize*8,
};

/* Read from the first fd, do its own work, and write to the second */
typedef void (*StreamFilter)(int, int);

extern int blind;	/* no feedback for input, disables rawmode */
extern int linecontrol;

extern int debugging;
extern void enabledebug(const char *);
extern void debug(const char *, ...);

typedef struct Buffer Buffer;
struct Buffer
{
	uint32_t size;
	uint32_t written;
	uint32_t read;
	void (*add)(char, Buffer *);
	char *data;
};
extern Buffer* balloc(uint32_t);
extern uint32_t bwrite(Buffer *, char *, uint32_t);
extern char *bread(Buffer *, uint32_t *);
extern int blineready(Buffer *);
#define bempty(b) (b->read == b->written)
#define bspace(b) (b->size - b->written)
#define bpending(b) (b->written - b->read)

extern void servecons(char *argv[], StreamFilter, StreamFilter);

extern int fsinit(int *, int *);
extern void fsserve(int, char*);

extern void passthrough(int, int);

extern void writecga(int, int);
extern void readkeyboard(int, int);

