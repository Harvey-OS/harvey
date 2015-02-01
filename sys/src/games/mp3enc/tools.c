/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *   Simple LAME context free functions:
 *
 *     - should not contain gfc or gfp
 *     - should not call other functions not defined 
 *       in this module (or libc functions)
 *
 *   Otherwise util.c is the right place.
 */
