/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "common.h"

int8_t *MAILROOT =	"/mail";
int8_t *UPASLOG =		"/sys/log";
int8_t *UPASLIB = 	"/mail/lib";
int8_t *UPASBIN=		"/bin/upas";
int8_t *UPASTMP = 	"/mail/tmp";
int8_t *SHELL = 		"/bin/rc";
int8_t *POST =		"/sys/lib/post/dispatch";

int MBOXMODE = 0662;
