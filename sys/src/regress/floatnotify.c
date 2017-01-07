/*
 * This file is part of Harvey.
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

/* 
 * This is a test which uses flaoting point in a notify handler
 * currently this will fail with:
 * "floatnotify 89: suicide: sys: floating point in note handler"
 * if you remove this check from fpu.c, the kernel will panic
 * This is a test for issue #152
 */

void
handler(void *v, char *s)
{
	double x = 1.0;
	x = x * 100.3;
	print("PASS\n");
	exits("PASS");
}

void
main(void)
{
	void (*f)(void) = nil;
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}
	f();
	print("FAIL");
	exits("FAIL");
}
