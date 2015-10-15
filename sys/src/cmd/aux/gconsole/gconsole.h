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

enum
{
	InputBufferSize = 32,
	OutputBufferSize = InputBufferSize*8,
};

extern int stdio;
extern int rawmode;

extern void enabledebug(void);
extern void debug(const char *, ...);

extern void serve(int, int, int);

extern void writecga(int, int);

extern void readkeyboard(int, int, int);
