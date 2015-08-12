/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */
#ifdef HARVEYARM
/*
 * arm definition
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "/arm/include/ureg.h"
#include <mach.h>

#define REGOFF(x) (ulong)(&((struct Ureg*)0)->x)

#define SP REGOFF(r13)
#define PC REGOFF(pc)

#define REGSIZE sizeof(struct Ureg)

Reglist armreglist[] = {{"TYPE", REGOFF(type), RINT | RRDONLY, 'X'},
                        {"PSR", REGOFF(psr), RINT | RRDONLY, 'X'},
                        {"PC", PC, RINT, 'X'},
                        {"SP", SP, RINT, 'X'},
                        {"R15", PC, RINT, 'X'},
                        {"R14", REGOFF(r14), RINT, 'X'},
                        {"R13", REGOFF(r13), RINT, 'X'},
                        {"R12", REGOFF(r12), RINT, 'X'},
                        {"R11", REGOFF(r11), RINT, 'X'},
                        {"R10", REGOFF(r10), RINT, 'X'},
                        {"R9", REGOFF(r9), RINT, 'X'},
                        {"R8", REGOFF(r8), RINT, 'X'},
                        {"R7", REGOFF(r7), RINT, 'X'},
                        {"R6", REGOFF(r6), RINT, 'X'},
                        {"R5", REGOFF(r5), RINT, 'X'},
                        {"R4", REGOFF(r4), RINT, 'X'},
                        {"R3", REGOFF(r3), RINT, 'X'},
                        {"R2", REGOFF(r2), RINT, 'X'},
                        {"R1", REGOFF(r1), RINT, 'X'},
                        {"R0", REGOFF(r0), RINT, 'X'},
                        {0}};

/* the machine description */
Mach marm = {
    "arm", MARM,   /* machine type */
    armreglist,    /* register set */
    REGSIZE,       /* register set size */
    0,             /* fp register set size */
    "PC",          /* name of PC */
    "SP",          /* name of SP */
    "R14",         /* name of link register */
    "setR12",      /* static base register name */
    0,             /* static base register value */
    0x1000,        /* page size */
    0xC0000000ULL, /* kernel base */
    0xC0000000ULL, /* kernel text mask */
    0x3FFFFFFFULL, /* user stack top */
    4,             /* quantization of pc */
    4,             /* szaddr */
    4,             /* szreg */
    4,             /* szfloat */
    8,             /* szdouble */
};
#endif
