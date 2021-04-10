/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

typedef struct Ureg {
	u64 x0;
	u64 x1;
	u64 x2;
	u64 x3;
	u64 x4;
	u64 x5;
	u64 x6;
	u64 x7;
	u64 x8;
	u64 x9;
	u64 x10;
	u64 x11;
	u64 x12;
	u64 x13;
	u64 x14;
	u64 x15;
	union {
		u64 x16;
		u64 ip0;
	};
	union {
		u64 x17;
		u64 ip1;
	};
	u64 x18;
	u64 x19;
	u64 x20;
	u64 x21;
	u64 x22;
	u64 x23;
	u64 x24;
	u64 x25;
	u64 x26;
	u64 x27;
	u64 x28;
	union {
		u64 x29;
		u64 fp;
	};
	union {
		u64 x30;
		u64 lr;
	};
	u64 sp;
	u64 type;	// of exception
	u64 psr;
	u64 pc;	// interrupted addr
} Ureg;
