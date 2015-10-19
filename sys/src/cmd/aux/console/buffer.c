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

Buffer*
balloc(uint32_t size)
{
	Buffer* b;

	b = (Buffer*)malloc(sizeof(Buffer));
	b->written = 0;
	b->read = 0;
	b->size = size;
	b->data = (char*)malloc(size);

	return b;
}
uint32_t
bwrite(Buffer *b, char *source, uint32_t len)
{
	uint32_t i;
	char *dest;

	debug("bwrite(%#p, src, %d): b->written was %d; ", b, len, b->written);
	if(b->read == b->written){
		b->written = 0;
		b->read = 0;
	}
	if(len > b->size - b->written)
		len = b->size - b->written;
	if(len > 0){
		dest = b->data + b->written;
		for(i = 0; i < len; ++i)
			*dest++ = *source++;
		b->written += len;
	}
	debug("len = %d, b->written = %d, b->size = %d, b->read = %d\n", len, b->written, b->size, b->read);
	return len;
}
void
bdelete(Buffer *b, uint32_t len)
{
	if(len > b->written - b->read)
		len = b->written - b->read;
	b->written -= len;
}
char *
bread(Buffer *b, uint32_t *length)
{
	uint32_t len;
	char *data;

	len = *length;
	data = nil;
	debug("bread(%#p, %#p): len = %d, b->written was %d, b->read was %d; ", b, length, len, b->written, b->read);
	if(len > b->written - b->read)
		len = b->written - b->read;
	assert(len < b->size);	/* check correctness by detecting overflows */
	if(len > 0){
		data = b->data + b->read;
		b->read += len;
	}
	debug("len = %d, b->written = %d, b->size = %d, b->read = %d\n", len, b->written, b->size, b->read);
	*length = len;
	return data;
}
