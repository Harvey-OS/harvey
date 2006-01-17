#ifndef _ARCH_H
#define _ARCH_H
#ifdef T386
#include "386.h"
#elif Tmips
#include "mips.h"
#elif Tpower
#include "mips.h"
#elif Talpha
#include "alpha.h"
#elif Tarm
#include "arm.h"
#elif Tamd64
#include "amd64.h"
#else
	I do not know about your architecture.
	Update switch in arch.h with new architecture.
#endif	/* T386 */
#endif /* _ARCH_H */
