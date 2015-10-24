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

/* data buffers */
Buffer*
balloc(uint32_t size)
{
	Buffer* b;

	b = (Buffer*)malloc(sizeof(Buffer));
	b->written = 0;
	b->read = 0;
	b->linewidth = 0;
	b->newlines = 0;
	b->ctrld = size;
	b->size = size;
	b->data = (char*)malloc(size);

	return b;
}
uint32_t
bwrite(Buffer *b, char *source, uint32_t len)
{
	uint32_t i;

	debug("bwrite(%#p, src, %d): b->written was %d, b->ctrld was %d, b->linewidth was %d; ", b, len, b->written, b->ctrld, b->linewidth);
	i = 0;
	if(b->read == b->written){
		b->written = 0;
		b->read = 0;
	}
	while(i < len)
	{
		if(b->written < b->size){
			b->add(source[i++], b);
		} else
			len = i;
	}

	debug("len = %d, b->written = %d, b->size = %d, b->read = %d, b->ctrld = %d, b->linewidth = %d\n", len, b->written, b->size, b->read, b->ctrld, b->linewidth);
	return len;
}
char *
bread(Buffer *b, uint32_t *length)
{
	uint32_t len;
	char *data;

	len = *length;
	data = nil;
	debug("bread(%#p, %#p): len = %d, b->written was %d, b->read was %d, b->ctrld was %d; ", b, length, len, b->written, b->read, b->ctrld);
	if(len > b->written - b->read)
		len = b->written - b->read;
	if(b->written > b->ctrld)
		len = b->ctrld - b->read;
	assert(len < b->size);	/* check correctness by detecting overflows */
	if(len > 0){
		data = b->data + b->read;
		b->read += len;
	}
	if(b->read == b->ctrld)
		b->ctrld = b->size;
	debug("len = %d, b->written = %d, b->size = %d, b->read = %d, b->ctrld = %d\n", len, b->written, b->size, b->read, b->ctrld);
	*length = len;
	return data;
}
char *
breadln(Buffer *b, uint32_t *length)
{
	uint32_t len;
	char *data, *c;

	len = *length;
	data = nil;
	debug("breadln(%#p, %#p): len = %d, b->written was %d, b->read was %d, b->ctrld was %d; ", b, length, len, b->written, b->read, b->ctrld);
	if(len > b->written - b->read)
		len = b->written - b->read;
	if(b->written > b->ctrld)
		len = b->ctrld - b->read;
	assert(len < b->size);	/* check correctness by detecting overflows */
	if(len > 0){
		data = b->data + b->read;
		/* take up to the first \n... */
		c = data;
		while(*c != '\n' && c - data < len)
			++c;
		if(c < b->data + b->ctrld)
			++c;	/* include \n, but not ^D */
		len = c - data;
		--b->newlines;
		b->linewidth = 0;
		b->read += len;
	}
	if(b->read == b->ctrld)
		b->ctrld = b->size;
	debug("len = %d, b->written = %d, b->size = %d, b->read = %d, b->ctrld = %d\n", len, b->written, b->size, b->read, b->ctrld);
	*length = len;
	return data;
}
int
blineready(Buffer *b)
{
	if(b && !bempty(b)){
		if(b->ctrld < b->size)
			return 1;	/* ctrl-d, see cons(3) */
		if(b->newlines > 0)
			return 1;
	}
	return 0;
}
