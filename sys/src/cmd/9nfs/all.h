/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <auth.h>
#include <authsrv.h>
#include <fcall.h>
#include <regexp.h>
#include "dat.h"
#include "fns.h"
#include "rpc.h"
#include "nfs.h"
#pragma varargck type "I" uint32_t
#pragma varargck argpos chat 1
#pragma varargck argpos clog 1
#pragma varargck argpos panic 1
