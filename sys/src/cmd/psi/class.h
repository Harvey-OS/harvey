# define X	 0

# define NUMBER	1
# define EXP	2	/* exponent	*/
# define SIGN	3	/* +-		*/
# define DOT	4	/* .		*/
# define REG	5	/* regular	*/
# define POUND	6	/* #		*/

# define LBRC	7	/* {		*/
# define LBRK	8	/* [		*/
# define LPAR	9	/* (		*/
# define LT	10	/* <		*/
# define PERC	11	/* %		*/
# define RBRC	12	/* }		*/
# define RBRK	13	/* ]		*/
# define SLASH	14	/* /		*/
# define WS	15	/* whitespace	*/
# define E_OF	17	/* end of file	*/
# define ERROR	18	/* control	*/

extern unsigned char map[];
enum	token
{
	TK_OTHER = -1,
	TK_NONE ,
	TK_ERROR1 ,
	TK_ERROR2 ,
	TK_ERROR3 ,
	TK_LBRACE ,
	TK_RBRACE ,
	TK_STRING ,
	TK_INTEGER ,
	TK_REAL ,
	TK_RADIXNUM ,
	TK_LBRACKET ,
	TK_RBRACKET ,
	TK_NAME ,
	TK_IMMNAME ,
	TK_LITNAME ,
	TK_HEXSTRING ,
	TK_EOF ,
} ;
struct object get_token(FILE *, int *) ;
