/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015  Giacomo Tesio
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

#include <u.h>
#include <libc.h>

int
awakened(int64_t wakeup)
{
	/* awake returns the ticks of the scheduled wakeup in negative,
	 * thus a wakeup is in the past iff (-awake(0)) >= (-wakeup)
	 *
	 * NOTE: this is not a macro so that we can change the awake()
	 * implementation in the future, without affecting the client code.
	 */
	assert(wakeup < 0);
	return wakeup >= awake(0);
}

int
forgivewkp(int64_t wakeup)
{
	/* awake returns the ticks of the scheduled wakeup in negative,
	 * and is able to remove a wakeup provided such value.
	 *
	 * forgivewkp() is just a wrapper to hide awake()'s details that
	 * could change in the future and make client code easier to
	 * read.
	 *
	 * NOTE: this is not a macro so that we can change the awake()
	 * implementation in the future, without affecting the client code.
	 */
	assert(wakeup < 0);
	return awake(wakeup);
}
