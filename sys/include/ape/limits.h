#ifndef __LIMITS
#define __LIMITS
/* 8 bit chars (signed), 16 bit shorts, 32 bit ints/longs */

#define CHAR_BIT	8
#define MB_LEN_MAX	3

#define UCHAR_MAX	0xff
#define USHRT_MAX	0xffff
#define UINT_MAX	0xffffffffU
#define ULONG_MAX	0xffffffffUL

#define CHAR_MAX	SCHAR_MAX
#define SCHAR_MAX	0x7f
#define SHRT_MAX	0x7fff
#define INT_MAX		0x7fffffff
#define LONG_MAX	0x7fffffffL

#define CHAR_MIN	SCHAR_MIN
#define SCHAR_MIN	(-SCHAR_MAX-1)
#define SHRT_MIN	(-SHRT_MAX-1)
#define INT_MIN		(-INT_MAX-1)
#define LONG_MIN	(-LONG_MAX-1)

#ifdef _POSIX_SOURCE

#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	6
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	0
#define _POSIX_OPEN_MAX		16
#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512
#define _POSIX_SSIZE_MAX	32767
#define _POSIX_STREAM_MAX	8
#define _POSIX_TZNAME_MAX	3

/*#define ARG_MAX	_POSIX_ARG_MAX		/* pedagogy */
/*#define CHILD_MAX	_POSIX_CHILD_MAX	/* pedagogy */
/*#define LINK_MAX	_POSIX_LINK_MAX		/* pedagogy */
/*#define MAX_CANON	_POSIX_MAX_CANON	/* pedagogy */
/*#define MAX_INPUT	_POSIX_MAX_INPUT	/* pedagogy */
/*#define NAME_MAX	_POSIX_NAME_MAX		/* pedagogy */
#define NGROUPS_MAX	_POSIX_NGROUPS_MAX
/*#define OPEN_MAX	_POSIX_OPEN_MAX		/* pedagogy */
/*#define PATH_MAX	_POSIX_PATH_MAX		/* pedagogy */
/*#define PIPE_BUF	_POSIX_PIPE_BUF		/* pedagogy */
#define SSIZE_MAX	LONG_MAX
/*#define STREAM_MAX	_POSIX_STREAM_MAX	/* pedagogy */
#define TZNAME_MAX	_POSIX_TZNAME_MAX

#ifdef _LIMITS_EXTENSION
/* some things are just too big for pedagogy (X!) */
#include <sys/limits.h>
#endif
#endif /* _POSIX_SOURCE */

#endif /* __LIMITS */
