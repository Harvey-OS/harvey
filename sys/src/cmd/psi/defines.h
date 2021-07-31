# define NEL(x)			( sizeof(x) / sizeof(x[0]) )
# define SAVEITEM(x)		saveitem(&x,sizeof(x))

# define MAXNEG			(1<<31)
# define INTEGER		2147483647

# define OPERAND_STACK_LIMIT	500
# define DICT_STACK_LIMIT	20
# define EXEC_STACK_LIMIT	250

# define DICTIONARY_LIMIT	65535
# define ARRAY_LIMIT		65535
# define STRING_LIMIT		15000
# define NAME_LIMIT		128

# define USER_DICT_LIMIT	200
# define DASH_LIMIT		11
# define GSAVE_LEVEL_LIMIT	31
# define SAVE_LEVEL_LIMIT	15
# define PATH_LIMIT		1500

# define FLAT			1
# define CTM_SIZE		6
# define GRAYLEVELS		256
#define GRAYIMP	17

# define FONT_DICT_LIMIT	250

#define SMALL .0001
#define SMALLDIF	.000001

#ifdef BSD
#define PI	3.1415926535897932384626433832795028841971693993751
#endif
#ifndef SYS_V
#define M_PI	PI
#define srand48	srand
#define lrand48	lrand
#endif
