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
#pragma	varargck	type	"I"	ulong
#pragma	varargck	argpos	chat	1
#pragma	varargck	argpos	clog	1
#pragma	varargck	argpos	panic	1
