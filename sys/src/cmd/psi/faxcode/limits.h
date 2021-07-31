/* Extreme values of integral types (conforms to Oct 1, 86 draft ANSI C std) */
#define CHAR_BIT 8
#define SCHAR_MIN -128
#define SCHAR_MAX 127
#define UCHAR_MIN 0
#define UCHAR_MAX 255
#define CHAR_MIN SCHAR_MIN	/* type char sign-extends */
#define CHAR_MAX SCHAR_MAX	/* type char sign-extends */

#define SHRT_MIN -32768
#define SHRT_MAX 32767
#define USHRT_MIN 0
#define USHRT_MAX 65535

#define INT_MIN -2147483648
#define INT_MAX 2147483647
#define UINT_MIN 0
#define UINT_MAX 4294967295

#define LONG_MIN -2147483648
#define LONG_MAX 2147483647
#define ULONG_MIN 0
#define ULONG_MAX 4294967295
