#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <io.h>
#include <setjmp.h>
#include <direct.h>
#include <process.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

/* disable various silly warnings */
#ifdef MSVC
#pragma warning( disable : 4245 4305 4244 4102 4761 4090 4028 4024)
#endif

typedef __int64		p9_vlong;
typedef	unsigned __int64 p9_uvlong;
typedef unsigned uintptr;
