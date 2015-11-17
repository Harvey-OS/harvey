/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "gzwrite.c"

int
uopen(const char* path, int flags, int mode)
{
	int n;
	int32_t f;
	
	f = flags&O_ACCMODE;
	if(flags&O_CREAT){
		if(access(path, 0) >= 0){
			if(flags&O_EXCL){
				return -1;
			}else{
				if((flags&O_TRUNC)&&(flags&(O_RDWR|O_WRONLY)))
					f |= 16;
				n = open(path, f);
			}
		}else{
			n = create(path, f, mode&0777);
		}
		if(n < 0)
			return n;
	}else{
		if((flags&O_TRUNC)&&(flags&(O_RDWR|O_WRONLY)))
			f |= 16;
		n = open(path, f);
		if(n < 0)
			return n;
	}
	if(n >= 0){
		if(flags&O_APPEND)
			seek(n, 0, 2);
	}
	return n;
}

int ZEXPORTVA gzvprintfhack(gzFile file, const char *format, va_list va)
{
    int size, len;
    gz_statep state;
    z_streamp strm;

    /* get internal structure */
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    strm = &(state->strm);

    /* check that we're writing and that there's no error */
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return 0;

    /* make sure we have some buffer space */
    if (state->size == 0 && gz_init(state) == -1)
        return 0;

    /* check for seek request */
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return 0;
    }

    /* consume whatever's left in the input buffer */
    if (strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
        return 0;

    /* do the printf() into the input buffer, put length in len */
    size = (int)(state->size);
    state->in[size - 1] = 0;
    len = vsnprintf((char *)(state->in), size, format, va);

    /* check that printf() results fit in buffer */
    if (len <= 0 || len >= (int)size || state->in[size - 1] != 0)
        return 0;

    /* update buffer and position, defer compression until needed */
    strm->avail_in = (unsigned)len;
    strm->next_in = state->in;
    state->x.pos += len;
    return len;
}

int ZEXPORTVA gzprintf(gzFile file, const char *format, ...)
{
    va_list va;
    int ret;

    va_start(va, format);
    ret = gzvprintfhack(file, format, va);
    va_end(va);
    return ret;
}
