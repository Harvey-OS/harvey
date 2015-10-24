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
#include <u.h>
#include <libc.h>

#include "console.h"

enum {
	Black		= 0x00,
	Blue		= 0x01,
	Green		= 0x02,
	Cyan		= 0x03,
	Red		= 0x04,
	Magenta		= 0x05,
	Brown		= 0x06,
	Grey		= 0x07,

	Bright 		= 0x08,
	Blinking	= 0x80,

	kbAttr		= (Black<<4)|Grey,	/* (background<<4)|foreground */
	Cursor = (Grey<<4)|Black,
};

enum {
	Width		= 80*2,
	Height		= 25,
};

static int cgapos;
static uint8_t CGA[16384];

static void
cgaputc(uint8_t c)
{
	int i;
	uint8_t *cga, *p;

	cga = CGA;

	cga[cgapos] = ' ';
	cga[cgapos+1] = kbAttr;

	switch(c){
	case '\r':
		break;
	case '\n':
		cgapos = cgapos/Width;
		cgapos = (cgapos+1)*Width;
		break;
	case '\t':
		/* while we could calculate the appropriate column here
		 *
		 * - we can't decides how many characters to delete on \b
		 * - it's pointless: who cares in 2015?
		 */
		cgaputc(' ');
		break;
	case '\b':
		if(cgapos >= 2)
			cgapos -= 2;
		break;
	default:
		cga[cgapos++] = c;
		cga[cgapos++] = kbAttr;
		break;
	}

	if(cgapos >= (Width*Height)){
		memmove(cga, &cga[Width], Width*(Height-1));
		p = &cga[Width*(Height-1)];
		for(i = 0; i < Width/2; i++){
			*p++ = ' ';
			*p++ = kbAttr;
		}

		cgapos -= Width;
	}

	cga[cgapos] = ' ';
	cga[cgapos+1] = Cursor;
}

static int32_t
cgaflush(int device)
{
	int32_t w;
	Dir *dirp;

	w = 1;	/* 0 must mean eof, thus this is the default */
	if((dirp = dirfstat(device)) != nil){
		if(dirp->length == 0)
			w = pwrite(device, CGA, Width*Height, 0);
		free(dirp);
	} else {
		w = pwrite(device, CGA, Width*Height, 0);
	}
	return w;
}

/* read from conswrites, write to device */
void
writecga(int conswrites, int device)
{
	int32_t pid, r, w, i;
	static uint8_t buf[ScreenBufferSize];

	pid = getpid();

	debug("writecga %d: started\n", pid);
	do
	{
		r = read(conswrites, buf, ScreenBufferSize);
		debug("writecga %d: read(conswrites) returns %d\n", pid, r);
		for(i = 0; i < r; i++)
			cgaputc(buf[i]);
		w = cgaflush(device);
		debug("writecga %d: cgaflush(device) returns %d\n", pid, w);
	}
	while(r > 0 && w > 0);

	close(conswrites);
	debug("writecga %d: close(%d)\n", pid, conswrites);
	close(device);
	debug("writecga %d: close(%d)\n", pid, device);

	debug("writecga %d: shut down (r = %d, w = %d)\n", pid, r, w);
	if(r < 0)
		exits("read");
	if(w < 0)
		exits("write");
	exits(nil);
}
