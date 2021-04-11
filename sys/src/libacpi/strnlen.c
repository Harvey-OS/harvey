/*
 * Copyright (c) 2005-2014 Rich Felker, et al.
 * Copyright (c) 2015-2016 HarveyOS et al.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE.mit file.
 */

usize strnlen(const char *s, usize n)
{
	const char *p = memchr(s, 0, n);
	return p ? p-s : n;
}
