#include <string.h>
#include "utf.h"

#define nil ((void*)0)

#define uchar _fmtuchar
#define ushort _fmtushort
#define uint _fmtuint
#define ulong _fmtulong
#define vlong _fmtvlong
// #define uvlong _fmtuvlong

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef unsigned long long uvlong;

#define OREAD O_RDONLY
#define OWRITE O_WRONLY
#define ORDWR O_RDWR
#define OCEXEC 0
