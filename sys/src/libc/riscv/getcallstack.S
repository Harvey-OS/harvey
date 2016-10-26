// This file is part of the Harvey operating system.  It is subject to the
// license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
// directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
//
// No part of Harvey operating system, including this file, may be copied,
// modified, propagated, or distributed except according to the terms
// contained in the LICENSE.gpl file.

.text

.globl getcallstack
getcallstack:
	li a0, 0
	li a1, 0
	RET
